#pragma once

#include <fstream>
#include <vector>

namespace util {
    /*
    * @brief Reads n bytes from given offset (default 0)
    * @note the file reference will be modified
    * */
    std::vector<char> get_n_bytes_from_file(std::ifstream &file, uint64_t num_bytes,
                                            uint64_t offset) {
        std::vector<char> buffer(num_bytes);

        // auto old_offset = file.tellg();

        file.seekg(offset);
        file.read(buffer.data(), num_bytes);

        // file.seekg( old_offset );	// restore original offset

        return buffer;
    }
} // namespace util
