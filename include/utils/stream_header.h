#ifndef STREAM_HEADER_H
#define STREAM_HEADER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace StreamHeader {

enum class ModelType : uint8_t {
    ORDER_0 = 0,
    ORDER_1 = 1,
    ORDER_2 = 2,
    ORDER_0_BWT = 3,
    ORDER_1_BWT = 4,
    ORDER_0_PREPROC = 5,
    ORDER_1_PREPROC = 6,
    UNCOMPRESSED = 255
};

enum TransformFlag : uint8_t {
    TRANSFORM_BWT = 1 << 0,
    TRANSFORM_MTF = 1 << 1,
    TRANSFORM_ZRLE = 1 << 2
};

struct Header {
    ModelType model_type;
    uint64_t original_size;
    uint64_t preprocessed_size;
    uint8_t transform_flags;
    std::vector<uint32_t> bwt_primary_indices;
    size_t payload_offset;

    bool uses_bwt() const;
    bool uses_mtf() const;
    bool uses_zrle() const;
    bool is_order0() const;
    bool is_order1() const;
};

bool has_flag(uint8_t flags, TransformFlag flag);
void set_flag(uint8_t& flags, TransformFlag flag);

void write_header(std::vector<uint8_t>& buffer,
                  ModelType model_type,
                  uint64_t original_size,
                  uint64_t preprocessed_size,
                  uint8_t transform_flags,
                  const std::vector<uint32_t>& bwt_primary_indices);

Header parse_header(const std::vector<uint8_t>& data);

std::string describe_model_type(const Header& header);

} // namespace StreamHeader

#endif // STREAM_HEADER_H
