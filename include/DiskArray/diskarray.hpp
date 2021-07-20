#include <vector>
#include <string>
#include <fstream>
#include <mutex>
#include <cstdio>
#include <thread>
#include <filesystem>

#if defined(DISKARRAY_LOG_SAVES)
	#define DISKARRAY_DEBUG
#endif

#ifdef DISKARRAY_DEBUG
#include <iostream>
#endif

#include "file.hpp"
#include "sfinae.hpp"

/*
 * Requirements for `T` :
 * 1. Must provide a 'bool SerializeToOstream(fstream*)' method, or something with convertible arguments
 * */
template <typename T, long MAX_NUM_ELEMENTS_IN_MEMORY = -1>
class DiskArray {
	using ElementType =  std::remove_pointer_t< std::remove_reference_t< T > >;

	/* #ifndef MAX_NUM_ELEMENTS_IN_MEMORY	// if it's forced by programmer, use that value
	 static constexpr size_t MAX_NUM_ELEMENTS_IN_MEMORY = 1000000 / sizeof(T);	// ie. Max approx 1MB by default
	#endif */

	static inline std::mutex _file_mutex;	// since std::tmpnam() isn't threadsafe

	std::string m_file_name;
	std::fstream m_file;
	std::fstream m_index_file;	// a binary file, simply containing array of 64-bit integers

	std::vector<ElementType> _m_arr;
	size_t m_num_objects_file { 0 };	// number of elements in file

	long _approx_byte_size { 0 };
	uint32_t _approx_byte_size_counter = 0;	// skip num turns to re-sync value of _approx_byte_size (so not do O(N) checking at each push_back())

	// void _flush_writes();	// flush all previous writes
	/*
	 * Stores a pointer, and a bool (whether it is element owned by someone else, or a pointer owned by itself only)
	 * Problem with std::reference_wrapper -> Can point to element in array but can not free a new element created on heap
	 * Problem with smart pointers -> When pointing to element in vector, it will double free, since actual owner should be vector
	 * 
	 * So this is a hybrid solution- Deallocate only if owned by self, and not if an element in a vector
	 **/
	class _RefType {
		ElementType* m_ptr;
		bool owned;
		public:
		operator ElementType& () noexcept {	// conversion operator, to be able to use as a reference
			return *m_ptr;
		}
		_RefType(ElementType* ptr, bool is_already_owned): m_ptr(ptr), owned(is_already_owned) {}
		~_RefType() {
			if (owned) {
				delete m_ptr;
			}
		}
	};

	_RefType _GetIndexInFile(size_t i);

	public:
	/*
	 * @param reserve_size 	   (Default=0) Corresponds to vector::vector(reserved_size) constructor
	 * @param file_directory   (Default=$(pwd)) Location to keep the 'temporary' index and array files
	 * */
	DiskArray(size_t reserve_size = 0, const std::string& file_directory = ".");
	~DiskArray();

	_RefType at(size_t i);
	void push_back(const ElementType&);

	void _move_num_elements_from_begin_to_file(size_t);
};

template<typename T, long MAX_NUM_ELEMENTS_IN_MEMORY>
DiskArray<T, MAX_NUM_ELEMENTS_IN_MEMORY>::DiskArray(size_t reserve_size, const std::string& file_directory ) {
	static_assert( util::has_SerializeToOstream<ElementType>::value, "Type Must provide a 'bool SerializeToOstream(fstream*)' method" );
	static_assert( MAX_NUM_ELEMENTS_IN_MEMORY == -1 && util::has_SpaceUsed<ElementType>::value, "Type Must provide a 'long SpaceUsedLong()' method, to work with automatic guess to when the usage limit is reached and should be reduced" );

	{
		// the tmpnam() + file creation should complete before this is repeated again
		// Why not use tmpfile() -> Because on Linux it will create in /tmp which is tmpfs on many systems, so no benefits
		std::lock_guard<std::mutex> lock(DiskArray<T>::_file_mutex);

		char tmp_path[L_tmpnam];
		std::tmpnam(tmp_path);

		// get a temporary filename, and append it to the file_directory path
		m_file_name = std::filesystem::path( tmp_path ).filename();
		m_file_name = std::filesystem::path( file_directory ).append( m_file_name );

		m_file.open( m_file_name, std::ios::out );	// create file
		m_file.close();
		m_file.open( m_file_name );	// then open again in r/w

		std::string index_filename{ m_file_name.append(".index") };

		#ifdef DISKARRAY_DEBUG
		std::clog << "Created: " << m_file_name << "\nAnd: " << index_filename << std::endl;
		#endif

		m_index_file.open( index_filename, std::ios::out | std::ios::binary );	// create file
		m_index_file.close();
		m_index_file.open( index_filename, std::ios::binary | std::ios::in | std::ios::out );
	}

	_m_arr.reserve(reserve_size);
}

template<typename T, long MAX_NUM_ELEMENTS_IN_MEMORY>
DiskArray<T, MAX_NUM_ELEMENTS_IN_MEMORY>::~DiskArray() {
	m_file.close();
	m_index_file.close();

	#ifndef DISKARRAY_KEEP_FILES
	std::remove( m_file_name );
	std::string index_filename( m_file_name );
	index_filename.append(".index");

	std::remove( index_filename.c_str() );
	#endif
}

template<typename T, long MAX_NUM_ELEMENTS_IN_MEMORY>
void DiskArray<T, MAX_NUM_ELEMENTS_IN_MEMORY>::_move_num_elements_from_begin_to_file(size_t n) {
	// Earlier i though to use flush, and save to disk on a different thread, but now we do it by blocking here itself... usme aur complexities hai, for eg. what if flush called multiple times ? And then requires a convar or a simple mutex

	#ifdef DISKARRAY_LOG_SAVES
	std::clog << "Moving " << n << " entries to file\n";
	#endif

	n = std::min( n, _m_arr.size() );	// if n>0, move all elements

	// TODO: FUTURE: Do this on another thread so as not to block here, then flush such pending operation when .at is called
	for(auto i=0; i<n; ++i) {
		uint64_t new_object_location = m_file.tellp();
		m_index_file.write( reinterpret_cast<char*>(&new_object_location), sizeof(uint64_t) );

		_m_arr[i].SerializeToOstream( &m_file );
	}

	m_num_objects_file += n;

	_approx_byte_size = 0;
	_approx_byte_size_counter = 0;
	_m_arr.erase( _m_arr.begin(), _m_arr.begin() + n );
}

template<typename T, long MAX_NUM_ELEMENTS_IN_MEMORY>
void DiskArray<T, MAX_NUM_ELEMENTS_IN_MEMORY>::push_back(const ElementType& e) {
	static size_t size_limit = 20000000;	// 10^7 (20 MB) bytes max in memory

	if constexpr ( MAX_NUM_ELEMENTS_IN_MEMORY == -1 ) {
		/* auto decide
		 * decides on basis of size, instead of memory
		 * , is at worst O(N) algorithm */
		if ( _approx_byte_size > size_limit/2 && _approx_byte_size_counter-- == 0 ) /*mark unlikely*/ {
			// recompute _approx_byte_size
			_approx_byte_size = 0;

			for (const auto& obj: _m_arr ) {
				_approx_byte_size += obj.SpaceUsedLong();
			}

			_approx_byte_size_counter = (size_limit / (_approx_byte_size/_m_arr.size())) / 4;	// skip reverify for next 10 calls, except the obvious case when _approx_byte_size itself crosses limit
		}

		if ( _approx_byte_size > size_limit ) {
			_approx_byte_size_counter = 0;	// don't skip next check whenever needed

			_move_num_elements_from_begin_to_file( _m_arr.size() );
		}

	} else if (_m_arr.size() >= MAX_NUM_ELEMENTS_IN_MEMORY) {
		_move_num_elements_from_begin_to_file( MAX_NUM_ELEMENTS_IN_MEMORY );
	}

	_approx_byte_size += e.SpaceUsedLong();
	_m_arr.push_back(e);
}

template<typename T, long MAX_NUM_ELEMENTS_IN_MEMORY>
typename DiskArray<T, MAX_NUM_ELEMENTS_IN_MEMORY>::_RefType DiskArray<T, MAX_NUM_ELEMENTS_IN_MEMORY>::at(size_t i) {
	// _flush_writes();	// flush all previous writes, before accessing an index, as it 'maybe' in the file
	if( i >= _m_arr.size() + m_num_objects_file ) {	// OUT OF RANGE
		throw std::out_of_range("Out Of Range");
	}

	if( i > m_num_objects_file ) {	// ie. in memory
		return _RefType( &_m_arr[ i-m_num_objects_file ], true );
	} else {
		// fetch from file
		return _GetIndexInFile( i );
	}
}

template<typename T, long MAX_NUM_ELEMENTS_IN_MEMORY>
typename DiskArray<T, MAX_NUM_ELEMENTS_IN_MEMORY>::_RefType DiskArray<T, MAX_NUM_ELEMENTS_IN_MEMORY>::_GetIndexInFile(size_t i) {
	uint64_t index_in_file;
	uint64_t next_valid_index;

	// FUTURE: NOT MULTITHREAD SAFE
	m_index_file.seekg( i * sizeof( uint64_t ) );	// index file is basically array of 64 bit indexes
	m_index_file.read( reinterpret_cast<char*>( &index_in_file ), sizeof(uint64_t));
	m_index_file.read( reinterpret_cast<char*>( &next_valid_index ), sizeof(uint64_t));

	m_file.seekg( index_in_file );

	const auto object_size = next_valid_index - index_in_file;
	char* object = new char[ object_size ];
	m_file.read( object, next_valid_index - index_in_file );

	T* obj = new T;	// allocating on heap
	obj->ParseFromString( std::string(object, object + object_size ) );	/*CAUTION: If just passed the char*, then the string constructor will consider it as a C string, ie. ends with a '\0' character, which can be any character in binary, not necessarily the end of the data*/

	return _RefType{ obj, false };
}
