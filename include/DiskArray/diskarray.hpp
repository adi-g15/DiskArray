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
template <typename T>
class DiskArray {
	using ElementType =  std::remove_pointer_t< std::remove_reference_t< T > >;
	using idx = int_fast32_t;

	#ifndef MAX_NUM_ELEMENTS_IN_MEMORY	// if it's forced by programmer, use that value
	static constexpr size_t MAX_NUM_ELEMENTS_IN_MEMORY = 1000000 / sizeof(T);	// ie. Max approx 1MB by default
	#endif

	static inline std::mutex _file_mutex;	// since std::tmpnam() isn't threadsafe

	std::string m_file_name;
	std::fstream m_file;
	std::fstream m_index_file;	// a binary file, simply containing array of 64-bit integers
	idx m_num_elements_file { 0 };	// number of elements in file

	std::vector<ElementType> _m_arr;

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

	_RefType get_nth_element_from_file();

	public:
	/*
	 * Returning pointer instead of reference
	 *
	 * Since, the object may or maynot be present in RAM
	 *
	 * In case it's present in RAM, we return its address, else create the object and return it's pointer
	 *
	 * TODO: PROBLEM with this is, we shouldn't just return a smart pointer to a element in the vector, find some other way for this, TODO: DECIDE LATER
	 * 
	 * @param reserve_size 	   (Default=0) Corresponds to vector::vector(reserved_size) constructor
	 * @param file_directory   (Default=$(pwd)) Location to keep the 'temporary' index and array files
	 * */
	DiskArray(size_t reserve_size = 0, const std::string& file_directory = ".");
	~DiskArray();

	_RefType at(idx i);
	void push_back(const ElementType&);

	void _move_num_elements_from_begin_to_file(size_t);	/* Intentionally making it public, but telling the intention of private with '_' */
};

template<typename T>
DiskArray<T>::DiskArray(size_t reserve_size, const std::string& file_directory ) {
	static_assert( util::has_SerializeToOstream<ElementType>::value, "Type Must provide a 'bool SerializeToOstream(fstream*)' method" );

	{
		// the tmpnam() + file creation should complete before this is repeated again
		// Why not use tmpfile() -> Because on Linux it will create in /tmp which is tmpfs on many systems, so no benefits
		std::lock_guard<std::mutex> lock(DiskArray<T>::_file_mutex);

		char tmp_path[L_tmpnam];
		std::tmpnam(tmp_path);

		// the next two lines, get a temporary filename, and append it to the file_directory path
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

template<typename T>
DiskArray<T>::~DiskArray() {
	m_file.close();
	m_index_file.close();

	#ifndef DISKARRAY_KEEP_FILES
	std::remove( m_file_name );
	std::string index_filename( m_file_name );
	index_filename.append(".index");

	std::remove( index_filename.c_str() );
	#endif
}

template<typename T>
void DiskArray<T>::_move_num_elements_from_begin_to_file(size_t n) {
	// Earlier i though to use flush, and save to disk on a different thread, but now we do it by blocking here itself... usme aur complexities hai, for eg. what if flush called multiple times ? And then requires a convar or a simple mutex

	n = std::min( n, _m_arr.size() );	// if n>0, move all elements

	// TODO: FUTURE: Do this on another thread so as not to block here, then flush such pending operation when .at is called
	for(auto i=0; i<n; ++i) {
		// TODO: new_object_location is current write location or is it +1 ?
		uint64_t new_object_location = m_file.tellp();
		m_index_file.write( reinterpret_cast<char*>(&new_object_location), sizeof(uint64_t) );

		_m_arr[i].SerializeToOstream( &m_file );
	}

	_m_arr.erase( _m_arr.begin(), _m_arr.begin() + n );
}

template<typename T>
void DiskArray<T>::push_back(const ElementType& e) {
	if(_m_arr.size() >= MAX_NUM_ELEMENTS_IN_MEMORY) {
		_move_num_elements_from_begin_to_file( MAX_NUM_ELEMENTS_IN_MEMORY );
	}

	_m_arr.push_back(e);
}

template<typename T>
typename DiskArray<T>::_RefType DiskArray<T>::at(DiskArray<T>::idx i) {
	// _flush_writes();	// flush all previous writes, before accessing an index, as it 'maybe' in the file


}
