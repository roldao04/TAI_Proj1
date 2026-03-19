#include <iostream>
#include <algorithm>
#include <chrono>
#include <string>
#include <cstring>
#include <thread>
#include <utility>
#include "arithmetic/range_coder.h"
#include "arithmetic/rans_static.h"
#include "model/frequency_model.h"
#include "model/context_model.h"
#include "utils/file_io.h"
#include "utils/entropy_calculator.h"
#include "utils/stream_header.h"
#include "transform/bwt.h"
#include "transform/mtf.h"
#include "transform/zero_rle.h"

/*
 * Lossless Data Compressor
 *
 * Compressed file format:
 * - Model type (1 byte): legacy tags 0/1/3/4 and extended preprocessing tags 5/6
 * - Original file size (8 bytes, uint64_t)
 * - Extended preprocessing header (if using MTF and/or ZRLE)
 * - BWT header (if BWT enabled): block count + primary indices
 * - Model data (Order-0 only: frequency table)
 * - Range-coded data
 */

using StreamHeader::ModelType;

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <input_file> <output_file> [OPTIONS]" << std::endl;
    std::cerr << "\nModel Options:" << std::endl;
    std::cerr << "  --model auto      Auto-select best model based on file analysis (default)" << std::endl;
    std::cerr << "  --model order0    Force Order-0 frequency model (fast, universal)" << std::endl;
    std::cerr << "  --model order1    Force Order-1 adaptive model (best compression for low-entropy data)" << std::endl;
    std::cerr << "\nBWT Preprocessing:" << std::endl;
    std::cerr << "  --bwt             Force BWT preprocessing (improves compression on structured data)" << std::endl;
    std::cerr << "  --no-bwt          Disable BWT preprocessing (default: auto-decided)" << std::endl;
    std::cerr << "\nPost-BWT Transforms:" << std::endl;
    std::cerr << "  --mtf             Force Move-to-Front after BWT" << std::endl;
    std::cerr << "  --no-mtf          Disable Move-to-Front after BWT" << std::endl;
    std::cerr << "  --zrle            Force zero-run RLE after MTF" << std::endl;
    std::cerr << "  --no-zrle         Disable zero-run RLE after MTF" << std::endl;
    std::cerr << "\nOther Options:" << std::endl;
    std::cerr << "  --yes, -y         Skip interactive prompts (useful for automation/benchmarks)" << std::endl;
    std::cerr << "\nAuto-selection rules (based on benchmark results):" << std::endl;
    std::cerr << "  Entropy > 7.5 → UNCOMPRESSED (incompressible, e.g., already compressed)" << std::endl;
    std::cerr << "  Entropy 7.2-7.5 → rANS Order-0 (very high entropy, Order-1 gain marginal)" << std::endl;
    std::cerr << "  File < 100KB → rANS Order-0 (adaptive overhead not worth it)" << std::endl;
    std::cerr << "  Otherwise → Order-1 (entropy <= 7.2, achieves best compression)" << std::endl;
    std::cerr << "\nNotes:" << std::endl;
    std::cerr << "  - Order-1 uses simplified encoding (no PPM Method C exclusions)" << std::endl;
    std::cerr << "  - Order-2 removed (provided no benefit over Order-1)" << std::endl;
    std::cerr << "  - MTF defaults to enabled when BWT is enabled" << std::endl;
    std::cerr << "  - ZRLE defaults to auto: enabled only if it shrinks the MTF output" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string input_filename = argv[1];
    std::string output_filename = argv[2];

    ModelType model_type = ModelType::ORDER_0;
    bool auto_select = true;
    bool force_mode = false;
    int bwt_preference = 0;
    int mtf_preference = 0;
    int zrle_preference = 0;

    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--yes" || arg == "-y") {
            force_mode = true;
        } else if (arg == "--bwt") {
            bwt_preference = 1;
        } else if (arg == "--no-bwt") {
            bwt_preference = -1;
        } else if (arg == "--mtf") {
            mtf_preference = 1;
        } else if (arg == "--no-mtf") {
            mtf_preference = -1;
        } else if (arg == "--zrle") {
            zrle_preference = 1;
        } else if (arg == "--no-zrle") {
            zrle_preference = -1;
        } else if (arg == "--model") {
            if (i + 1 < argc) {
                std::string model_name = argv[i + 1];
                if (model_name == "order0") {
                    model_type = ModelType::RANS_ORDER_0;
                    auto_select = false;
                } else if (model_name == "order1") {
                    model_type = ModelType::ORDER_1;
                    auto_select = false;
                } else if (model_name == "order2") {
                    std::cerr << "Error: Order-2 has been removed (provided no benefit over Order-1)" << std::endl;
                    std::cerr << "       Benchmark results showed identical compression ratios with worse performance" << std::endl;
                    std::cerr << "       Use --model order1 instead, or --model auto for automatic selection" << std::endl;
                    return 1;
                } else if (model_name == "auto") {
                    auto_select = true;
                } else {
                    std::cerr << "Error: Unknown model type '" << model_name << "'" << std::endl;
                    print_usage(argv[0]);
                    return 1;
                }
                i++;
            } else {
                std::cerr << "Error: --model requires an argument (order0, order1, or auto)" << std::endl;
                print_usage(argv[0]);
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        std::cout << "Reading input file: " << input_filename << std::endl;
        std::vector<uint8_t> input_data = FileIO::read_file(input_filename);
        std::cout << "Input size: " << input_data.size() << " bytes" << std::endl;

        if (auto_select) {
            double entropy = EntropyCalculator::calculate(input_data, 8192);
            std::cout << "Detected entropy: " << entropy << " bits/symbol" << std::endl;

            if (entropy > 7.5) {
                model_type = ModelType::UNCOMPRESSED;
                std::cout << "Decision: UNCOMPRESSED (entropy " << entropy << " > 7.5, incompressible)" << std::endl;
            } else if (entropy > 7.2) {
                model_type = ModelType::RANS_ORDER_0;
                std::cout << "Decision: rANS Order-0 (high entropy " << entropy << " > 7.2, Order-1 gain marginal)" << std::endl;
            } else if (input_data.size() < 102400) {
                model_type = ModelType::RANS_ORDER_0;
                std::cout << "Decision: rANS Order-0 (small file < 100KB)" << std::endl;
            } else {
                model_type = ModelType::ORDER_1;
                std::cout << "Decision: Order-1 (entropy " << entropy << " <= 7.2, good compression expected)" << std::endl;
            }
        } else {
            std::cout << "Using user-specified model" << std::endl;
        }

        if (!auto_select && model_type == ModelType::ORDER_1) {
            double entropy = EntropyCalculator::calculate(input_data, 8192);
            if (entropy > 7.2) {
                std::cerr << "\n⚠️  WARNING: Order-1 not recommended for very high-entropy files!" << std::endl;
                std::cerr << "    Detected entropy: " << entropy << " bits/symbol (threshold: 7.2)" << std::endl;
                std::cerr << "    Expected issues: Marginal compression gain over rANS Order-0." << std::endl;
                std::cerr << "    Recommendation: Use --model auto (automatic selection) or --model order0" << std::endl;

                if (force_mode) {
                    std::cerr << "    --yes flag detected: Proceeding with Order-1 despite warning" << std::endl;
                } else {
                    std::cerr << "\nContinue with Order-1 anyway? (y/N): ";
                    char response;
                    std::cin >> response;
                    if (response != 'y' && response != 'Y') {
                        std::cout << "Aborted. Using Order-0 instead for safety." << std::endl;
                        model_type = ModelType::ORDER_0;
                    }
                }
            }
        }

        bool use_bwt = false;
        bool use_mtf = false;
        bool use_zrle = false;
        uint8_t transform_flags = 0;
        std::vector<uint32_t> bwt_primary_indices;
        std::vector<uint8_t> data_to_encode = input_data;

        if (model_type != ModelType::UNCOMPRESSED) {
            if (bwt_preference == 1) {
                use_bwt = true;
                std::cout << "BWT preprocessing: ENABLED (user requested --bwt)" << std::endl;
            } else if (bwt_preference == -1) {
                use_bwt = false;
                std::cout << "BWT preprocessing: DISABLED (user requested --no-bwt)" << std::endl;
            } else {
                double entropy = EntropyCalculator::calculate(input_data, 8192);
                if (entropy < 6.5 && input_data.size() > 10240) {
                    use_bwt = true;
                    std::cout << "BWT preprocessing: ENABLED (entropy " << entropy << " < 6.5, structured data expected)" << std::endl;
                } else {
                    use_bwt = false;
                    std::cout << "BWT preprocessing: DISABLED (entropy " << entropy << " or size not suitable)" << std::endl;
                }
            }

            if (!use_bwt) {
                if (mtf_preference == 1) {
                    throw std::runtime_error("--mtf requires BWT preprocessing");
                }
                if (zrle_preference == 1) {
                    throw std::runtime_error("--zrle requires BWT preprocessing");
                }
            }
        }

        uint64_t original_size = input_data.size();

        if (use_bwt) {
            use_mtf = (mtf_preference != -1);
            if (zrle_preference == 1) {
                if (mtf_preference == -1) {
                    throw std::runtime_error("--zrle requires MTF preprocessing; remove --no-mtf or disable --zrle");
                }
                use_mtf = true;
            }

            // PARALLEL PATH: BWT+MTF+ZRLE+Order-1 independently per block
            if (use_mtf && model_type == ModelType::ORDER_1) {
                std::cout << "Using parallel per-block pipeline (BWT+MTF+ZRLE+Order-1 per block)..." << std::endl;

                // Dynamic block sizing: divide file into even blocks,
                // targeting at most MAX_BLOCK_SIZE bytes per block.
                const size_t MAX_BLOCK_SIZE = 2000 * 1024;
                size_t num_blocks = std::max<size_t>(1,
                    (input_data.size() + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE);
                const size_t BLOCK_SIZE = (input_data.size() + num_blocks - 1) / num_blocks;

                std::vector<StreamHeader::ParallelBlockMeta> block_metas(num_blocks);
                std::vector<std::vector<uint8_t>> block_compressed(num_blocks);

                auto compress_one_block = [&](size_t bi) {
                    size_t blk_start = bi * BLOCK_SIZE;
                    size_t blk_end   = std::min(blk_start + BLOCK_SIZE, input_data.size());
                    std::vector<uint8_t> block(input_data.begin() + blk_start,
                                               input_data.begin() + blk_end);

                    auto [bwt_data, primary_index] = BWT::transform(block);
                    std::vector<uint8_t> mtf_data  = MoveToFront::transform(bwt_data);

                    std::vector<uint8_t> zrle_data = ZeroRunLengthEncoder::encode(mtf_data);
                    bool has_rank255 = std::any_of(mtf_data.begin(), mtf_data.end(),
                                                   [](uint8_t b){ return b == 255; });
                    bool use_zrle_this = !has_rank255 && (zrle_preference != -1) &&
                                        (zrle_preference == 1 || zrle_data.size() < mtf_data.size());
                    const std::vector<uint8_t>& to_encode = use_zrle_this ? zrle_data : mtf_data;

                    ContextModel ctx_model;
                    ctx_model.set_encoding_method_ppm_c();
                    ctx_model.init_adaptive();
                    RangeEncoder range_enc;
                    for (uint8_t b : to_encode) {
                        auto res = ctx_model.encode_symbol_fast(b);
                        for (int si = 0; si < res.count; si++) {
                            range_enc.encode_symbol(res.steps[si].cum_freq_low,
                                                    res.steps[si].cum_freq_high,
                                                    res.steps[si].total_freq);
                        }
                        ctx_model.update_frequencies(b);
                        ctx_model.update_history(b);
                    }
                    range_enc.finish();

                    StreamHeader::ParallelBlockMeta meta;
                    meta.bwt_primary_index       = primary_index;
                    meta.transform_flags         = StreamHeader::TRANSFORM_BWT | StreamHeader::TRANSFORM_MTF;
                    if (use_zrle_this) meta.transform_flags |= StreamHeader::TRANSFORM_ZRLE;
                    meta.original_block_size      = static_cast<uint32_t>(block.size());
                    meta.preprocessed_block_size  = static_cast<uint32_t>(to_encode.size());
                    meta.compressed_block_size    = static_cast<uint32_t>(range_enc.get_output().size());

                    block_metas[bi]     = meta;
                    block_compressed[bi] = range_enc.get_output();
                };

                std::vector<std::thread> threads;
                threads.reserve(num_blocks);
                for (size_t i = 0; i < num_blocks; i++) {
                    threads.emplace_back(compress_one_block, i);
                }
                for (auto& t : threads) t.join();

                std::vector<uint8_t> output_data;
                StreamHeader::write_parallel_header(output_data, original_size, block_metas);
                for (const auto& bc : block_compressed) {
                    output_data.insert(output_data.end(), bc.begin(), bc.end());
                }

                std::cout << "Writing compressed file: " << output_filename << std::endl;
                FileIO::write_file(output_filename, output_data);

                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

                std::cout << "\n=== Compression Statistics ===" << std::endl;
                std::cout << "Model: Parallel BWT+MTF+ZRLE+Order-1 (" << num_blocks << " blocks)" << std::endl;
                std::cout << "Original size:    " << input_data.size() << " bytes" << std::endl;
                std::cout << "Compressed size:  " << output_data.size() << " bytes" << std::endl;
                std::cout << "Compression ratio: " << (100.0 * output_data.size() / input_data.size()) << "%" << std::endl;
                std::cout << "Bits per symbol:  " << (8.0 * output_data.size() / input_data.size()) << std::endl;
                std::cout << "Compression time: " << duration.count() << " ms" << std::endl;

                return 0;
            }
            // else fall through to sequential path

            std::cout << "Applying BWT preprocessing..." << std::endl;
            auto start_bwt = std::chrono::high_resolution_clock::now();

            auto [bwt_output, primary_indices] = BWT::transform_blocks(input_data, 1024*1024);
            data_to_encode = std::move(bwt_output);
            bwt_primary_indices = primary_indices;
            StreamHeader::set_flag(transform_flags, StreamHeader::TRANSFORM_BWT);

            auto end_bwt = std::chrono::high_resolution_clock::now();
            auto bwt_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_bwt - start_bwt).count();
            std::cout << "BWT preprocessing complete (" << bwt_time << " ms)" << std::endl;
            std::cout << "Number of BWT blocks: " << bwt_primary_indices.size() << std::endl;

            if (use_mtf) {
                std::cout << "Applying Move-to-Front transform..." << std::endl;
                auto start_mtf = std::chrono::high_resolution_clock::now();
                data_to_encode = MoveToFront::transform_blocks(data_to_encode, 1024*1024);
                auto end_mtf = std::chrono::high_resolution_clock::now();
                auto mtf_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_mtf - start_mtf).count();
                std::cout << "Move-to-Front transform complete (" << mtf_time << " ms)" << std::endl;
                StreamHeader::set_flag(transform_flags, StreamHeader::TRANSFORM_MTF);
            }

            if (use_mtf) {
                std::vector<uint8_t> zrle_candidate = ZeroRunLengthEncoder::encode(data_to_encode);
                if (zrle_preference == 1 ||
                    (zrle_preference == 0 && zrle_candidate.size() < data_to_encode.size())) {
                    use_zrle = true;
                    std::cout << "Zero-run RLE: ENABLED ("
                              << data_to_encode.size() << " -> "
                              << zrle_candidate.size() << " bytes)" << std::endl;
                    data_to_encode = std::move(zrle_candidate);
                    StreamHeader::set_flag(transform_flags, StreamHeader::TRANSFORM_ZRLE);
                } else if (zrle_preference == -1) {
                    std::cout << "Zero-run RLE: DISABLED (user requested --no-zrle)" << std::endl;
                } else {
                    std::cout << "Zero-run RLE: DISABLED (no size reduction after MTF)" << std::endl;
                }
            }
        }

        if (zrle_preference == 1 && !use_zrle) {
            throw std::runtime_error("--zrle could not be enabled because BWT/MTF preprocessing was not active");
        }

        if (use_bwt && use_mtf) {
            if (model_type == ModelType::ORDER_0 || model_type == ModelType::RANS_ORDER_0) {
                model_type = ModelType::ORDER_0_PREPROC;
            } else if (model_type == ModelType::ORDER_1) {
                model_type = ModelType::ORDER_1_PREPROC;
            }
        } else if (use_bwt) {
            if (model_type == ModelType::ORDER_0 || model_type == ModelType::RANS_ORDER_0) {
                model_type = ModelType::ORDER_0_BWT;
            } else if (model_type == ModelType::ORDER_1) {
                model_type = ModelType::ORDER_1_BWT;
            }
        }

        std::vector<uint8_t> output_data;
        StreamHeader::write_header(output_data,
                                   model_type,
                                   original_size,
                                   data_to_encode.size(),
                                   transform_flags,
                                   bwt_primary_indices);

        if (model_type == ModelType::RANS_ORDER_0) {
            std::cout << "Using rANS Order-0 (static, zero-division decode)..." << std::endl;

            // Build raw frequency table (257 symbols: 0-255 + EOF)
            std::array<uint32_t, RansStaticCoder::ALPHABET> raw_freq{};
            for (uint8_t b : data_to_encode) raw_freq[b]++;
            raw_freq[RansStaticCoder::EOF_SYMBOL] = 1;  // ensure EOF present

            RansStaticCoder rans;
            std::array<uint16_t, RansStaticCoder::ALPHABET> scaled = rans.build_encode_table(raw_freq);
            std::vector<uint8_t> rans_stream = rans.encode(data_to_encode);

            // Write scaled frequencies (257 × 2 bytes) then rANS bitstream
            output_data.push_back(static_cast<uint8_t>(RansStaticCoder::SCALE_BITS));
            for (int i = 0; i < RansStaticCoder::ALPHABET; i++) {
                output_data.push_back(scaled[i] & 0xFF);
                output_data.push_back((scaled[i] >> 8) & 0xFF);
            }
            output_data.insert(output_data.end(), rans_stream.begin(), rans_stream.end());

        } else if (model_type == ModelType::ORDER_0 ||
            model_type == ModelType::ORDER_0_BWT ||
            model_type == ModelType::ORDER_0_PREPROC) {
            std::cout << "Using Order-0 frequency model..." << std::endl;
            std::cout << "Encoding..." << std::endl;

            FrequencyModel model;
            model.build_from_data(data_to_encode);

            RangeEncoder encoder;
            for (uint8_t byte : data_to_encode) {
                uint32_t cum_freq_low, cum_freq_high;
                model.get_symbol_range(byte, cum_freq_low, cum_freq_high);
                encoder.encode_symbol(cum_freq_low, cum_freq_high, model.get_total_freq());
            }

            uint32_t cum_freq_low, cum_freq_high;
            model.get_symbol_range(FrequencyModel::get_eof_symbol(), cum_freq_low, cum_freq_high);
            encoder.encode_symbol(cum_freq_low, cum_freq_high, model.get_total_freq());

            encoder.finish();
            const std::vector<uint8_t>& encoded_data = encoder.get_output();

            const auto& frequencies = model.get_frequencies();
            for (int i = 0; i < 257; i++) {
                uint32_t freq = frequencies[i];
                for (int j = 0; j < 4; j++) {
                    output_data.push_back((freq >> (j * 8)) & 0xFF);
                }
            }

            output_data.insert(output_data.end(), encoded_data.begin(), encoded_data.end());

        } else if (model_type == ModelType::ORDER_1 && !use_bwt) {
            // PARALLEL ORDER-1 (no BWT): split into independent blocks, encode in parallel.
            // Each block gets a fresh Order-1 model; decompressor uses existing parallel path.
            // 512 KB blocks: E → 2 threads, F → 4 threads → ~2–4× faster decompression.
            const size_t MAX_BLOCK_NOBWT = 512 * 1024;
            size_t num_blocks = std::max<size_t>(1,
                (input_data.size() + MAX_BLOCK_NOBWT - 1) / MAX_BLOCK_NOBWT);
            const size_t BLOCK_SIZE = (input_data.size() + num_blocks - 1) / num_blocks;

            std::cout << "Using parallel Order-1 (" << num_blocks << " blocks, no BWT)..." << std::endl;

            std::vector<StreamHeader::ParallelBlockMeta> block_metas(num_blocks);
            std::vector<std::vector<uint8_t>> block_compressed(num_blocks);

            auto encode_one_block = [&](size_t bi) {
                size_t blk_start = bi * BLOCK_SIZE;
                size_t blk_end   = std::min(blk_start + BLOCK_SIZE, input_data.size());
                std::vector<uint8_t> block(input_data.begin() + blk_start,
                                           input_data.begin() + blk_end);

                ContextModel ctx_model;
                ctx_model.set_encoding_method_ppm_c();
                ctx_model.init_adaptive();
                RangeEncoder range_enc;

                for (uint8_t b : block) {
                    auto res = ctx_model.encode_symbol_fast(b);
                    for (int si = 0; si < res.count; si++) {
                        range_enc.encode_symbol(res.steps[si].cum_freq_low,
                                                res.steps[si].cum_freq_high,
                                                res.steps[si].total_freq);
                    }
                    ctx_model.update_frequencies(b);
                    ctx_model.update_history(b);
                }
                range_enc.finish();

                StreamHeader::ParallelBlockMeta meta;
                meta.bwt_primary_index      = 0;
                meta.transform_flags        = 0;  // no BWT/MTF/ZRLE
                meta.original_block_size    = static_cast<uint32_t>(block.size());
                meta.preprocessed_block_size = static_cast<uint32_t>(block.size());
                meta.compressed_block_size  = static_cast<uint32_t>(range_enc.get_output().size());

                block_metas[bi]      = meta;
                block_compressed[bi] = range_enc.get_output();
            };

            std::vector<std::thread> threads;
            threads.reserve(num_blocks);
            for (size_t i = 0; i < num_blocks; i++) {
                threads.emplace_back(encode_one_block, i);
            }
            for (auto& t : threads) t.join();

            std::vector<uint8_t> output_data;
            StreamHeader::write_parallel_header(output_data, original_size, block_metas);
            for (const auto& bc : block_compressed) {
                output_data.insert(output_data.end(), bc.begin(), bc.end());
            }

            std::cout << "Writing compressed file: " << output_filename << std::endl;
            FileIO::write_file(output_filename, output_data);

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            std::cout << "\n=== Compression Statistics ===" << std::endl;
            std::cout << "Model: Parallel Order-1 (" << num_blocks << " blocks)" << std::endl;
            std::cout << "Original size:    " << input_data.size() << " bytes" << std::endl;
            std::cout << "Compressed size:  " << output_data.size() << " bytes" << std::endl;
            std::cout << "Compression ratio: " << (100.0 * output_data.size() / input_data.size()) << "%" << std::endl;
            std::cout << "Bits per symbol:  " << (8.0 * output_data.size() / input_data.size()) << std::endl;
            std::cout << "Compression time: " << duration.count() << " ms" << std::endl;

            return 0;

        } else if (model_type == ModelType::ORDER_1 ||
                   model_type == ModelType::ORDER_2 ||
                   model_type == ModelType::ORDER_1_BWT ||
                   model_type == ModelType::ORDER_1_PREPROC) {
            std::string adaptive_model_name = (model_type == ModelType::ORDER_1 ||
                                               model_type == ModelType::ORDER_1_BWT ||
                                               model_type == ModelType::ORDER_1_PREPROC) ? "Order-1" : "Order-2";
            std::cout << "Using " << adaptive_model_name << " adaptive context model..." << std::endl;
            std::cout << "Encoding with simplified adaptive model..." << std::endl;

            ContextModel model;
            model.set_encoding_method_ppm_c();
            model.init_adaptive();

            RangeEncoder encoder;

            for (size_t idx = 0; idx < data_to_encode.size(); idx++) {
                uint8_t byte = data_to_encode[idx];

                auto res = model.encode_symbol_fast(byte);
                for (int si = 0; si < res.count; si++) {
                    encoder.encode_symbol(res.steps[si].cum_freq_low,
                                          res.steps[si].cum_freq_high,
                                          res.steps[si].total_freq);
                }

                model.update_frequencies(byte);
                model.update_history(byte);
            }

            encoder.finish();
            const std::vector<uint8_t>& encoded_data = encoder.get_output();

            output_data.insert(output_data.end(), encoded_data.begin(), encoded_data.end());
        } else if (model_type == ModelType::UNCOMPRESSED) {
            std::cout << "Storing UNCOMPRESSED (detected incompressible data)" << std::endl;
            output_data.insert(output_data.end(), input_data.begin(), input_data.end());
        }

        std::cout << "Writing compressed file: " << output_filename << std::endl;
        FileIO::write_file(output_filename, output_data);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // Statistics
        std::cout << "\n=== Compression Statistics ===" << std::endl;
        StreamHeader::Header header_summary{model_type, original_size, data_to_encode.size(), transform_flags, bwt_primary_indices, 0};
        std::cout << "Model: " << StreamHeader::describe_model_type(header_summary) << std::endl;
        std::cout << "Original size:    " << input_data.size() << " bytes" << std::endl;
        std::cout << "Compressed size:  " << output_data.size() << " bytes" << std::endl;
        std::cout << "Compression ratio: " << (100.0 * output_data.size() / input_data.size()) << "%" << std::endl;
        std::cout << "Bits per symbol:  " << (8.0 * output_data.size() / input_data.size()) << std::endl;
        std::cout << "Compression time: " << duration.count() << " ms" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
