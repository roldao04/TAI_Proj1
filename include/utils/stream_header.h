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
    PARALLEL = 7,       // fully independent per-block pipeline (BWT+MTF+ZRLE+Order1 per block)
    RANS_ORDER_0 = 8,   // rANS (ryg) static Order-0 — zero-division decode
    RANS_INTERLEAVED_BWT_MTF = 11, // v7: BWT + MTF + 2-way interleaved rANS Order-0 (speed)
    LZ77_FAST = 12,                // v8: LZ4-style LZ77 ultra-fast (no entropy coding)
    UNCOMPRESSED = 255
};

enum TransformFlag : uint8_t {
    TRANSFORM_BWT    = 1 << 0,
    TRANSFORM_MTF    = 1 << 1,
    TRANSFORM_ZRLE   = 1 << 2,
    TRANSFORM_LZP    = 1 << 3,
    TRANSFORM_WFC    = 1 << 4,  // WFC used instead of standard MTF
    TRANSFORM_ORDER2 = 1 << 5   // Order-2 context model active
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

struct ParallelBlockMeta {
    uint32_t bwt_primary_index;
    uint8_t  transform_flags;
    uint32_t original_block_size;
    uint32_t preprocessed_block_size;
    uint32_t compressed_block_size;
};

struct ParallelHeader {
    uint64_t original_size;
    std::vector<ParallelBlockMeta> blocks;
    size_t data_section_offset;
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

void write_parallel_header(std::vector<uint8_t>& buffer,
                           uint64_t original_size,
                           const std::vector<ParallelBlockMeta>& blocks);

ParallelHeader parse_parallel_header(const std::vector<uint8_t>& data);

std::string describe_model_type(const Header& header);

} // namespace StreamHeader

#endif // STREAM_HEADER_H
