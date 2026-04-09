#include <iostream>
#include <algorithm>
#include <chrono>
#include <string>
#include <cstring>
#include <thread>
#include "arithmetic/rans_static.h"
#include "utils/file_io.h"
#include "utils/entropy_calculator.h"
#include "utils/stream_header.h"
#include "transform/bwt.h"
#include "transform/mtf.h"
/*
 * v7.0 Speed-Optimized Lossless Compressor
 *
 * Pipeline: BWT (libsais, 4MB blocks) -> MTF -> ZRLE (if beneficial) -> Interleaved rANS Order-0
 *
 * Designed for maximum throughput with good compression ratio.
 *
 * File format:
 *   [0x0B] [original_size:8B] [block_count:4B]
 *   Per block metadata: [bwt_index:4B] [orig_block_size:4B] [compressed_size:4B]
 *   Per block data:     [scale_bits:1B] [scaled_freq:257x2B] [rANS stream:variable]
 *
 * bwt_index encoding:
 *   0xFFFFFFFF          = no BWT/MTF/ZRLE (raw rANS)
 *   bit 30 set (0x40000000) = ZRLE applied; bits 0-29 = BWT primary index
 *   otherwise           = BWT+MTF only; value = BWT primary index
 */

using StreamHeader::ModelType;

static constexpr size_t BLOCK_SIZE = 4 * 1024 * 1024;  // 4MB blocks
static constexpr uint32_t NO_BWT_SENTINEL = 0xFFFFFFFF; // sentinel: no BWT/MTF applied
static constexpr uint32_t ZRLE_FLAG = 0x40000000;       // bit 30: ZRLE was applied

struct BlockResult {
    uint32_t bwt_primary_index;  // NO_BWT_SENTINEL if raw rANS (no BWT/MTF)
    uint32_t original_block_size;
    uint32_t compressed_block_size;  // includes freq table + rANS stream
    std::array<uint16_t, RansStaticCoder::ALPHABET> scaled_freq;
    std::vector<uint8_t> rans_stream;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "G07 v7.0 - Speed-Optimized Lossless Compression\n\n";
        std::cout << "Usage: " << argv[0] << " <input_file> <output_file>\n\n";
        std::cout << "Pipeline: BWT + MTF + 2-way Interleaved rANS Order-0\n";
        std::cout << "Optimized for maximum speed with good compression.\n";
        return 1;
    }

    std::string input_filename = argv[1];
    std::string output_filename = argv[2];

    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> input_data = FileIO::read_file(input_filename);
        uint64_t original_size = input_data.size();
        std::cout << "Input: " << input_filename << " (" << original_size << " bytes)" << std::endl;

        if (original_size == 0) {
            // Empty file: write minimal header
            std::vector<uint8_t> output;
            output.push_back(static_cast<uint8_t>(ModelType::RANS_INTERLEAVED_BWT_MTF));
            for (int i = 0; i < 8; i++) output.push_back(0);  // original_size = 0
            uint32_t bc = 0;
            for (int i = 0; i < 4; i++) output.push_back((bc >> (i*8)) & 0xFF);
            FileIO::write_file(output_filename, output);
            std::cout << "Empty file compressed." << std::endl;
            return 0;
        }

        // Entropy check: if near-random, store uncompressed
        double entropy = EntropyCalculator::calculate(input_data, 8192);
        std::cout << "Entropy: " << entropy << " bits/symbol" << std::endl;

        if (entropy > 7.5) {
            std::cout << "Decision: UNCOMPRESSED (entropy > 7.5)" << std::endl;
            std::vector<uint8_t> output;
            output.push_back(static_cast<uint8_t>(ModelType::UNCOMPRESSED));
            for (int i = 0; i < 8; i++)
                output.push_back((original_size >> (i*8)) & 0xFF);
            output.insert(output.end(), input_data.begin(), input_data.end());
            FileIO::write_file(output_filename, output);

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cout << "Compressed: " << output.size() << " bytes ("
                      << (100.0 * output.size() / original_size) << "%)"
                      << " in " << duration.count() << " ms" << std::endl;
            return 0;
        }

        // Decide whether to use BWT+MTF (low entropy) or raw rANS (high entropy)
        bool use_bwt = (entropy < 6.5 && original_size > 10240);

        // Compute block layout
        size_t num_blocks = std::max<size_t>(1, (original_size + BLOCK_SIZE - 1) / BLOCK_SIZE);
        size_t block_size = (original_size + num_blocks - 1) / num_blocks;
        std::cout << "Pipeline: " << (use_bwt ? "BWT + MTF + " : "")
                  << "Interleaved rANS (" << num_blocks << " block(s))" << std::endl;

        std::vector<BlockResult> results(num_blocks);

        // Compress each block (threaded)
        auto compress_block = [&](size_t bi) {
            size_t blk_start = bi * block_size;
            size_t blk_end = std::min(blk_start + block_size, static_cast<size_t>(original_size));
            std::vector<uint8_t> block(input_data.begin() + blk_start,
                                       input_data.begin() + blk_end);

            uint32_t primary_index = NO_BWT_SENTINEL;
            const uint8_t* data_to_encode = block.data();
            size_t encode_len = block.size();
            std::vector<uint8_t> bwt_data, mtf_data;

            std::vector<uint8_t> zrle_data;

            if (use_bwt) {
                auto [bwt_out, bwt_idx] = BWT::transform(block);
                bwt_data = std::move(bwt_out);
                primary_index = bwt_idx;
                mtf_data = MoveToFront::transform(bwt_data);

                // Try ZRLE: apply only if it actually shrinks the data
                zrle_data = ZeroRunLengthEncoder::encode(mtf_data);
                if (zrle_data.size() < mtf_data.size()) {
                    primary_index |= ZRLE_FLAG;
                    data_to_encode = zrle_data.data();
                    encode_len = zrle_data.size();
                } else {
                    data_to_encode = mtf_data.data();
                    encode_len = mtf_data.size();
                }
            }

            // Count frequencies
            std::array<uint32_t, RansStaticCoder::ALPHABET> raw_freq{};
            for (size_t j = 0; j < encode_len; j++) raw_freq[data_to_encode[j]]++;
            raw_freq[RansStaticCoder::EOF_SYMBOL] = 1;

            // Build rANS table and encode
            RansStaticCoder rans;
            std::array<uint16_t, RansStaticCoder::ALPHABET> scaled = rans.build_encode_table(raw_freq);
            std::vector<uint8_t> rans_stream = rans.encode_interleaved(data_to_encode, encode_len);

            // Compressed block size = scale_bits(1) + freq_table(257*2) + rANS stream
            uint32_t compressed_size = 1 + RansStaticCoder::ALPHABET * 2
                                       + static_cast<uint32_t>(rans_stream.size());

            results[bi] = {
                primary_index,
                static_cast<uint32_t>(block.size()),
                compressed_size,
                scaled,
                std::move(rans_stream)
            };
        };

        if (num_blocks == 1) {
            compress_block(0);
        } else {
            std::vector<std::thread> threads;
            threads.reserve(num_blocks);
            for (size_t i = 0; i < num_blocks; i++)
                threads.emplace_back(compress_block, i);
            for (auto& t : threads) t.join();
        }

        // Assemble output
        std::vector<uint8_t> output;
        size_t estimated = 13 + num_blocks * 12;
        for (const auto& r : results)
            estimated += 1 + RansStaticCoder::ALPHABET * 2 + r.rans_stream.size();
        output.reserve(estimated);

        // Header: model type (1B)
        output.push_back(static_cast<uint8_t>(ModelType::RANS_INTERLEAVED_BWT_MTF));

        // Original size (8B, little-endian)
        for (int i = 0; i < 8; i++)
            output.push_back((original_size >> (i * 8)) & 0xFF);

        // Block count (4B)
        uint32_t bc = static_cast<uint32_t>(num_blocks);
        for (int i = 0; i < 4; i++)
            output.push_back((bc >> (i * 8)) & 0xFF);

        // Per-block metadata
        for (const auto& r : results) {
            for (int i = 0; i < 4; i++)
                output.push_back((r.bwt_primary_index >> (i * 8)) & 0xFF);
            for (int i = 0; i < 4; i++)
                output.push_back((r.original_block_size >> (i * 8)) & 0xFF);
            for (int i = 0; i < 4; i++)
                output.push_back((r.compressed_block_size >> (i * 8)) & 0xFF);
        }

        // Per-block data
        for (const auto& r : results) {
            // Scale bits (1B)
            output.push_back(static_cast<uint8_t>(RansStaticCoder::SCALE_BITS));
            // Scaled frequencies (257 x 2B)
            for (int i = 0; i < RansStaticCoder::ALPHABET; i++) {
                output.push_back(r.scaled_freq[i] & 0xFF);
                output.push_back((r.scaled_freq[i] >> 8) & 0xFF);
            }
            // rANS stream
            output.insert(output.end(), r.rans_stream.begin(), r.rans_stream.end());
        }

        FileIO::write_file(output_filename, output);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        double speed_mbps = (duration.count() > 0)
            ? (original_size / 1048576.0) / (duration.count() / 1000.0) : 0;

        std::cout << "\n=== Compression Statistics ===" << std::endl;
        std::cout << "Model: v7 BWT+MTF+Interleaved-rANS (" << num_blocks << " block(s))" << std::endl;
        std::cout << "Original size:    " << original_size << " bytes" << std::endl;
        std::cout << "Compressed size:  " << output.size() << " bytes" << std::endl;
        std::cout << "Compression ratio: " << (100.0 * output.size() / original_size) << "%" << std::endl;
        std::cout << "Bits per symbol:  " << (8.0 * output.size() / original_size) << std::endl;
        std::cout << "Compression time: " << duration.count() << " ms";
        if (speed_mbps > 0) std::cout << " (" << speed_mbps << " MB/s)";
        std::cout << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
