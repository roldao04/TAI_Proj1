#ifndef FILE_IO_H
#define FILE_IO_H

#include <vector>
#include <string>
#include <cstdint>

namespace FileIO {
    // Read entire file into a byte vector
    std::vector<uint8_t> read_file(const std::string& filename);

    // Write byte vector to file
    void write_file(const std::string& filename, const std::vector<uint8_t>& data);

    // Get file size in bytes
    size_t get_file_size(const std::string& filename);
}

#endif // FILE_IO_H
