#ifndef MTF_H
#define MTF_H

#include <cstddef>
#include <cstdint>
#include <vector>

class MoveToFront {
public:
    static std::vector<uint8_t> transform(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> inverse_transform(const std::vector<uint8_t>& input);

    static std::vector<uint8_t> transform_blocks(const std::vector<uint8_t>& input,
                                                 size_t block_size = 900 * 1024);
    static std::vector<uint8_t> inverse_transform_blocks(const std::vector<uint8_t>& input,
                                                         size_t block_size = 900 * 1024);
};

#endif // MTF_H
