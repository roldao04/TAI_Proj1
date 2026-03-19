#include "utils/stream_header.h"

#include <stdexcept>

namespace StreamHeader {

namespace {

void append_u32(std::vector<uint8_t>& buffer, uint32_t value) {
    for (int i = 0; i < 4; i++) {
        buffer.push_back((value >> (i * 8)) & 0xFF);
    }
}

void append_u64(std::vector<uint8_t>& buffer, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        buffer.push_back((value >> (i * 8)) & 0xFF);
    }
}

uint32_t read_u32(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) {
        throw std::runtime_error("Invalid compressed file: truncated uint32 field");
    }

    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value |= static_cast<uint32_t>(data[offset++]) << (i * 8);
    }
    return value;
}

uint64_t read_u64(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 8 > data.size()) {
        throw std::runtime_error("Invalid compressed file: truncated uint64 field");
    }

    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= static_cast<uint64_t>(data[offset++]) << (i * 8);
    }
    return value;
}

bool is_extended_model(ModelType model_type) {
    return model_type == ModelType::ORDER_0_PREPROC ||
           model_type == ModelType::ORDER_1_PREPROC;
}

bool is_legacy_bwt_model(ModelType model_type) {
    return model_type == ModelType::ORDER_0_BWT ||
           model_type == ModelType::ORDER_1_BWT;
}

void append_bwt_metadata(std::vector<uint8_t>& buffer,
                         const std::vector<uint32_t>& bwt_primary_indices) {
    append_u32(buffer, static_cast<uint32_t>(bwt_primary_indices.size()));
    for (uint32_t index : bwt_primary_indices) {
        append_u32(buffer, index);
    }
}

void read_bwt_metadata(const std::vector<uint8_t>& data,
                       size_t& offset,
                       std::vector<uint32_t>& bwt_primary_indices) {
    uint32_t block_count = read_u32(data, offset);
    bwt_primary_indices.clear();
    bwt_primary_indices.reserve(block_count);

    for (uint32_t i = 0; i < block_count; i++) {
        bwt_primary_indices.push_back(read_u32(data, offset));
    }
}

std::string describe_preprocessed_order(const Header& header, const std::string& base_name) {
    std::string description = base_name;

    if (!header.uses_bwt()) {
        return description;
    }

    description += " with BWT";
    if (header.uses_mtf()) {
        description += "+MTF";
    }
    if (header.uses_zrle()) {
        description += "+ZRLE";
    }

    return description;
}

} // namespace

bool Header::uses_bwt() const {
    return has_flag(transform_flags, TRANSFORM_BWT);
}

bool Header::uses_mtf() const {
    return has_flag(transform_flags, TRANSFORM_MTF);
}

bool Header::uses_zrle() const {
    return has_flag(transform_flags, TRANSFORM_ZRLE);
}

bool Header::is_order0() const {
    return model_type == ModelType::ORDER_0 ||
           model_type == ModelType::ORDER_0_BWT ||
           model_type == ModelType::ORDER_0_PREPROC;
}

bool Header::is_order1() const {
    return model_type == ModelType::ORDER_1 ||
           model_type == ModelType::ORDER_2 ||
           model_type == ModelType::ORDER_1_BWT ||
           model_type == ModelType::ORDER_1_PREPROC;
}

bool has_flag(uint8_t flags, TransformFlag flag) {
    return (flags & static_cast<uint8_t>(flag)) != 0;
}

void set_flag(uint8_t& flags, TransformFlag flag) {
    flags |= static_cast<uint8_t>(flag);
}

void write_header(std::vector<uint8_t>& buffer,
                  ModelType model_type,
                  uint64_t original_size,
                  uint64_t preprocessed_size,
                  uint8_t transform_flags,
                  const std::vector<uint32_t>& bwt_primary_indices) {
    buffer.push_back(static_cast<uint8_t>(model_type));
    append_u64(buffer, original_size);

    if (is_extended_model(model_type)) {
        buffer.push_back(transform_flags);
        append_u64(buffer, preprocessed_size);

        if (has_flag(transform_flags, TRANSFORM_BWT)) {
            append_bwt_metadata(buffer, bwt_primary_indices);
        }
        return;
    }

    if (is_legacy_bwt_model(model_type)) {
        append_bwt_metadata(buffer, bwt_primary_indices);
    }
}

Header parse_header(const std::vector<uint8_t>& data) {
    if (data.size() < 9) {
        throw std::runtime_error("Invalid compressed file: too small");
    }

    Header header;
    size_t offset = 0;
    header.model_type = static_cast<ModelType>(data[offset++]);
    header.original_size = read_u64(data, offset);
    header.preprocessed_size = header.original_size;
    header.transform_flags = 0;
    header.bwt_primary_indices.clear();

    if (header.model_type == ModelType::ORDER_0_BWT ||
        header.model_type == ModelType::ORDER_1_BWT) {
        set_flag(header.transform_flags, TRANSFORM_BWT);
        read_bwt_metadata(data, offset, header.bwt_primary_indices);
    } else if (is_extended_model(header.model_type)) {
        if (offset >= data.size()) {
            throw std::runtime_error("Invalid compressed file: missing transform flags");
        }

        header.transform_flags = data[offset++];
        header.preprocessed_size = read_u64(data, offset);

        if (has_flag(header.transform_flags, TRANSFORM_BWT)) {
            read_bwt_metadata(data, offset, header.bwt_primary_indices);
        }

        if (has_flag(header.transform_flags, TRANSFORM_MTF) &&
            !has_flag(header.transform_flags, TRANSFORM_BWT)) {
            throw std::runtime_error("Invalid compressed file: MTF requires BWT metadata");
        }

        if (has_flag(header.transform_flags, TRANSFORM_ZRLE) &&
            !has_flag(header.transform_flags, TRANSFORM_MTF)) {
            throw std::runtime_error("Invalid compressed file: ZRLE requires MTF");
        }
    } else if (header.model_type == ModelType::PARALLEL) {
        throw std::runtime_error("Use parse_parallel_header() for PARALLEL streams");
    } else if (header.model_type != ModelType::ORDER_0 &&
               header.model_type != ModelType::ORDER_1 &&
               header.model_type != ModelType::ORDER_2 &&
               header.model_type != ModelType::UNCOMPRESSED) {
        throw std::runtime_error("Unknown model type");
    }

    header.payload_offset = offset;
    return header;
}

void write_parallel_header(std::vector<uint8_t>& buffer,
                           uint64_t original_size,
                           const std::vector<ParallelBlockMeta>& blocks) {
    buffer.push_back(static_cast<uint8_t>(ModelType::PARALLEL));
    append_u64(buffer, original_size);
    append_u32(buffer, static_cast<uint32_t>(blocks.size()));
    for (const auto& b : blocks) {
        append_u32(buffer, b.bwt_primary_index);
        buffer.push_back(b.transform_flags);
        append_u32(buffer, b.original_block_size);
        append_u32(buffer, b.preprocessed_block_size);
        append_u32(buffer, b.compressed_block_size);
    }
}

ParallelHeader parse_parallel_header(const std::vector<uint8_t>& data) {
    if (data.size() < 13) {
        throw std::runtime_error("Invalid parallel compressed file: too small");
    }

    size_t offset = 0;
    if (static_cast<ModelType>(data[offset++]) != ModelType::PARALLEL) {
        throw std::runtime_error("Not a parallel compressed stream");
    }

    ParallelHeader ph;
    ph.original_size = read_u64(data, offset);
    uint32_t num_blocks = read_u32(data, offset);

    ph.blocks.resize(num_blocks);
    for (uint32_t i = 0; i < num_blocks; i++) {
        ph.blocks[i].bwt_primary_index    = read_u32(data, offset);
        ph.blocks[i].transform_flags      = data[offset++];
        ph.blocks[i].original_block_size   = read_u32(data, offset);
        ph.blocks[i].preprocessed_block_size = read_u32(data, offset);
        ph.blocks[i].compressed_block_size = read_u32(data, offset);
    }

    ph.data_section_offset = offset;
    return ph;
}

std::string describe_model_type(const Header& header) {
    switch (header.model_type) {
        case ModelType::ORDER_0:
            return "Order-0";
        case ModelType::ORDER_1:
            return "Order-1";
        case ModelType::ORDER_2:
            return "Order-2";
        case ModelType::ORDER_0_BWT:
        case ModelType::ORDER_0_PREPROC:
            return describe_preprocessed_order(header, "Order-0");
        case ModelType::ORDER_1_BWT:
        case ModelType::ORDER_1_PREPROC:
            return describe_preprocessed_order(header, "Order-1");
        case ModelType::PARALLEL:
            return "Parallel (per-block BWT+MTF+ZRLE+Order-1)";
        case ModelType::UNCOMPRESSED:
            return "Uncompressed";
    }

    return "Unknown";
}

} // namespace StreamHeader
