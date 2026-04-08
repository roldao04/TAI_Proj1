#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include "arithmetic/rans_static.h"
#include "utils/file_io.h"
#include "utils/stream_header.h"
#include "transform/bwt.h"
#include "transform/mtf.h"
#include "transform/zero_rle.h"

/*
 * v7.0 Speed-Optimized Lossless Decompressor
 *
 * Reverses: Interleaved rANS -> [ZRLE inverse ->] inverse MTF -> inverse BWT
 *
 * bwt_index encoding:
 *   0xFFFFFFFF          = no BWT/MTF/ZRLE (raw rANS)
 *   bit 30 set (0x40000000) = ZRLE applied; bits 0-29 = BWT primary index
 *   otherwise           = BWT+MTF only; value = BWT primary index
 */

using StreamHeader::ModelType;

static constexpr uint32_t NO_BWT_SENTINEL = 0xFFFFFFFF;
static constexpr uint32_t ZRLE_FLAG = 0x40000000;

struct BlockMeta {
    uint32_t bwt_primary_index;  // NO_BWT_SENTINEL = no BWT/MTF
    uint32_t original_block_size;
    uint32_t compressed_block_size;
};

static uint32_t read_u32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

static uint64_t read_u64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++)
        v |= static_cast<uint64_t>(p[i]) << (i * 8);
    return v;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "G07 v7.0 - Speed-Optimized Lossless Decompression\n\n";
        std::cout << "Usage: " << argv[0] << " <compressed_file> <output_file>\n\n";
        std::cout << "Handles v7 format (0x0B) and uncompressed (0xFF).\n";
        std::cout << "For v5 files: use g07-v5-d\n";
        return 1;
    }

    std::string input_filename = argv[1];
    std::string output_filename = argv[2];

    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> compressed = FileIO::read_file(input_filename);
        std::cout << "Input: " << input_filename << " (" << compressed.size() << " bytes)" << std::endl;

        if (compressed.empty())
            throw std::runtime_error("Empty compressed file");

        uint8_t model_byte = compressed[0];

        // Handle uncompressed
        if (static_cast<ModelType>(model_byte) == ModelType::UNCOMPRESSED) {
            if (compressed.size() < 9)
                throw std::runtime_error("Invalid uncompressed header");
            uint64_t orig_size = read_u64(compressed.data() + 1);
            std::vector<uint8_t> output(compressed.begin() + 9, compressed.end());
            if (output.size() != orig_size)
                std::cerr << "Warning: size mismatch" << std::endl;
            FileIO::write_file(output_filename, output);

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cout << "Decompressed: " << output.size() << " bytes in "
                      << duration.count() << " ms" << std::endl;
            return 0;
        }

        // Must be v7 format
        if (static_cast<ModelType>(model_byte) != ModelType::RANS_INTERLEAVED_BWT_MTF) {
            std::cerr << "Error: Not a v7 compressed file (model type: 0x"
                      << std::hex << (int)model_byte << std::dec << ")\n";
            std::cerr << "Use g07-v5-d for older formats.\n";
            return 1;
        }

        // Parse header
        if (compressed.size() < 13)
            throw std::runtime_error("Invalid v7 header: too small");

        size_t off = 1;
        uint64_t original_size = read_u64(compressed.data() + off); off += 8;
        uint32_t num_blocks = read_u32(compressed.data() + off); off += 4;

        std::cout << "Original size: " << original_size << " bytes, "
                  << num_blocks << " block(s)" << std::endl;

        if (original_size == 0) {
            FileIO::write_file(output_filename, {});
            std::cout << "Empty file decompressed." << std::endl;
            return 0;
        }

        // Read block metadata
        if (compressed.size() < off + num_blocks * 12)
            throw std::runtime_error("Invalid v7 header: truncated block metadata");

        std::vector<BlockMeta> metas(num_blocks);
        for (uint32_t i = 0; i < num_blocks; i++) {
            metas[i].bwt_primary_index   = read_u32(compressed.data() + off); off += 4;
            metas[i].original_block_size = read_u32(compressed.data() + off); off += 4;
            metas[i].compressed_block_size = read_u32(compressed.data() + off); off += 4;
        }

        size_t data_section_offset = off;

        // Decompress each block
        std::vector<std::vector<uint8_t>> block_outputs(num_blocks);

        auto decompress_block = [&](uint32_t bi) {
            // Locate this block's compressed data
            size_t blk_offset = data_section_offset;
            for (uint32_t j = 0; j < bi; j++)
                blk_offset += metas[j].compressed_block_size;

            const uint8_t* blk_data = compressed.data() + blk_offset;
            size_t blk_len = metas[bi].compressed_block_size;

            // Read scale_bits (1 byte)
            if (blk_len < 1 + RansStaticCoder::ALPHABET * 2)
                throw std::runtime_error("Block data too small");

            size_t boff = 0;
            uint8_t scale_bits = blk_data[boff++];
            if (scale_bits != RansStaticCoder::SCALE_BITS)
                throw std::runtime_error("Unsupported scale_bits");

            // Read scaled frequencies (257 x 2 bytes)
            std::array<uint16_t, RansStaticCoder::ALPHABET> scaled_freq{};
            for (int i = 0; i < RansStaticCoder::ALPHABET; i++) {
                scaled_freq[i] = static_cast<uint16_t>(blk_data[boff])
                               | (static_cast<uint16_t>(blk_data[boff + 1]) << 8);
                boff += 2;
            }

            // Build decode table
            RansStaticCoder rans;
            rans.build_decode_table(scaled_freq);

            // Decode interleaved rANS stream
            const uint8_t* rans_stream = blk_data + boff;
            size_t rans_len = blk_len - boff;
            size_t mtf_size = metas[bi].original_block_size;  // after BWT+MTF, same size

            std::vector<uint8_t> decoded(mtf_size);
            rans.decode_interleaved(rans_stream, rans_len, decoded.data(), mtf_size);

            if (metas[bi].bwt_primary_index != NO_BWT_SENTINEL) {
                // Inverse MTF then inverse BWT
                std::vector<uint8_t> bwt_data = MoveToFront::inverse_transform(decoded);
                decoded = BWT::inverse_transform(bwt_data, metas[bi].bwt_primary_index);
            }

            block_outputs[bi] = std::move(decoded);
        };

        if (num_blocks == 1) {
            decompress_block(0);
        } else {
            std::vector<std::thread> threads;
            threads.reserve(num_blocks);
            for (uint32_t i = 0; i < num_blocks; i++)
                threads.emplace_back(decompress_block, i);
            for (auto& t : threads) t.join();
        }

        // Concatenate blocks
        std::vector<uint8_t> output;
        output.reserve(original_size);
        for (const auto& bo : block_outputs)
            output.insert(output.end(), bo.begin(), bo.end());

        if (output.size() != original_size) {
            std::cerr << "Warning: Decoded size (" << output.size()
                      << ") doesn't match expected (" << original_size << ")" << std::endl;
        }

        FileIO::write_file(output_filename, output);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        double speed_mbps = (duration.count() > 0)
            ? (original_size / 1048576.0) / (duration.count() / 1000.0) : 0;

        std::cout << "\n=== Decompression Statistics ===" << std::endl;
        std::cout << "Compressed size:    " << compressed.size() << " bytes" << std::endl;
        std::cout << "Decompressed size:  " << output.size() << " bytes" << std::endl;
        std::cout << "Decompression time: " << duration.count() << " ms";
        if (speed_mbps > 0) std::cout << " (" << speed_mbps << " MB/s)";
        std::cout << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
