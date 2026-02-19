#include "utils/file_io.h"
#include <fstream>
#include <stdexcept>
#include <sstream>

namespace FileIO {

std::vector<uint8_t> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Cannot open file for reading: " + filename);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Error reading file: " + filename);
    }

    return buffer;
}

void write_file(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }

    if (!file.write(reinterpret_cast<const char*>(data.data()), data.size())) {
        throw std::runtime_error("Error writing file: " + filename);
    }
}

size_t get_file_size(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    return file.tellg();
}

} // namespace FileIO
