#include <iostream>
#include <chrono>
#include <array>
#include <thread>
#include "arithmetic/range_coder.h"
#include "arithmetic/rans_static.h"
#include "model/frequency_model.h"
#include "model/context_model.h"
#include "utils/file_io.h"
#include "utils/stream_header.h"
#include "transform/bwt.h"
#include "transform/lzp.h"
#include "transform/mtf.h"
#include "transform/x86_filter.h"
#include "transform/zero_rle.h"

/*
 * Lossless Data Decompressor
 *
 * Supports both legacy streams and the extended preprocessing format
 * used for BWT + MTF + optional zero-run RLE.
 */

using StreamHeader::ModelType;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "═══════════════════════════════════════════════════════\n";
        std::cout << "  G07 v6.0 - Fast Lossless Decompression\n";
        std::cout << "═══════════════════════════════════════════════════════\n";
        std::cout << "\nUsage: " << argv[0] << " <compressed_file> <output_file>\n\n";
        std::cout << "Handles v6 formats only (model types 0x00-0x08, 0xFF)\n";
        std::cout << "  • Order-0, Order-1, rANS Order-0\n";
        std::cout << "  • BWT + MTF + ZRLE variations\n";
        std::cout << "  • Uncompressed (0xFF)\n";
        std::cout << "\nFor v7 files: use g07-v7-d\n";
        std::cout << "For v8 files: use g07-v8-d\n";
        return 1;
    }

    std::string input_filename = argv[1];
    std::string output_filename = argv[2];

    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        std::cout << "Reading compressed file: " << input_filename << std::endl;
        std::vector<uint8_t> compressed_data = FileIO::read_file(input_filename);
        std::cout << "Compressed size: " << compressed_data.size() << " bytes" << std::endl;

        // Check if this is a v5 format (v7 is 0x09, v8 is 0x0A)
        if (!compressed_data.empty()) {
            uint8_t model_type_byte = compressed_data[0];
            if (model_type_byte == 0x09) {
                std::cerr << "\n❌ Error: This is a v7.0 compressed file (model type: 0x09)\n";
                std::cerr << "   Use g07-v7-d to decompress this file.\n";
                return 1;
            }
            if (model_type_byte == 0x0A) {
                std::cerr << "\n❌ Error: This is a v8.0 compressed file (model type: 0x0A)\n";
                std::cerr << "   Use g07-v8-d to decompress this file.\n";
                return 1;
            }
            if (model_type_byte > 0x0A && model_type_byte != 0xFF) {
                std::cerr << "\n❌ Error: Unknown format (model type: 0x"
                          << std::hex << (int)model_type_byte << std::dec << ")\n";
                std::cerr << "   This file may be from a newer version.\n";
                return 1;
            }
        }

        // Handle rANS Order-0 format
        if (!compressed_data.empty() &&
            static_cast<ModelType>(compressed_data[0]) == ModelType::RANS_ORDER_0) {

            if (compressed_data.size() < 1 + 8 + 1 + RansStaticCoder::ALPHABET * 2) {
                throw std::runtime_error("Invalid rANS compressed file: too small");
            }

            size_t off = 1;
            // Read original_size (8 bytes)
            uint64_t original_size = 0;
            for (int i = 0; i < 8; i++)
                original_size |= static_cast<uint64_t>(compressed_data[off++]) << (i * 8);

            std::cout << "Model type: rANS Order-0 (static)" << std::endl;
            std::cout << "Original size: " << original_size << " bytes" << std::endl;

            // Read scale_bits (1 byte)
            uint8_t scale_bits = compressed_data[off++];
            if (scale_bits != RansStaticCoder::SCALE_BITS) {
                throw std::runtime_error("rANS: unsupported scale_bits value");
            }

            // Read scaled frequencies (257 × 2 bytes, little-endian)
            std::array<uint16_t, RansStaticCoder::ALPHABET> scaled_freq{};
            for (int i = 0; i < RansStaticCoder::ALPHABET; i++) {
                scaled_freq[i] = static_cast<uint16_t>(compressed_data[off]) |
                                 (static_cast<uint16_t>(compressed_data[off + 1]) << 8);
                off += 2;
            }

            // Build decode table and decode
            RansStaticCoder rans;
            rans.build_decode_table(scaled_freq);

            std::vector<uint8_t> rans_stream(
                compressed_data.begin() + off, compressed_data.end());

            std::vector<uint8_t> output_data = rans.decode(rans_stream, original_size);

            if (output_data.size() != original_size) {
                std::cerr << "Warning: Decoded size (" << output_data.size()
                          << ") doesn't match expected size (" << original_size << ")" << std::endl;
            }

            std::cout << "Writing decompressed file: " << output_filename << std::endl;
            FileIO::write_file(output_filename, output_data);

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            std::cout << "\n=== Decompression Statistics ===" << std::endl;
            std::cout << "Compressed size:    " << compressed_data.size() << " bytes" << std::endl;
            std::cout << "Decompressed size:  " << output_data.size() << " bytes" << std::endl;
            std::cout << "Decompression time: " << duration.count() << " ms" << std::endl;

            return 0;
        }

        // Handle parallel format before the standard single-stream header parser
        if (!compressed_data.empty() &&
            static_cast<ModelType>(compressed_data[0]) == ModelType::PARALLEL) {

            StreamHeader::ParallelHeader ph = StreamHeader::parse_parallel_header(compressed_data);
            size_t num_blocks = ph.blocks.size();
            std::cout << "Model type: Parallel BWT-family + Order-1/2 (" << num_blocks << " blocks)" << std::endl;
            std::cout << "Original size: " << ph.original_size << " bytes" << std::endl;

            std::vector<std::vector<uint8_t>> block_outputs(num_blocks);

            auto decompress_one_block = [&](size_t bi) {
                const StreamHeader::ParallelBlockMeta& meta = ph.blocks[bi];
                uint32_t block_base_offset = 0;
                for (size_t j = 0; j < bi; j++) {
                    block_base_offset += ph.blocks[j].original_block_size;
                }

                size_t blk_offset = ph.data_section_offset;
                for (size_t j = 0; j < bi; j++) {
                    blk_offset += ph.blocks[j].compressed_block_size;
                }

                std::vector<uint8_t> encoded(
                    compressed_data.begin() + blk_offset,
                    compressed_data.begin() + blk_offset + meta.compressed_block_size);

                ContextModel ctx_model;
                ctx_model.set_encoding_method_simple();
                if (StreamHeader::has_flag(meta.transform_flags, StreamHeader::TRANSFORM_ORDER2))
                    ctx_model.enable_order2();
                ctx_model.init_adaptive();
                RangeDecoder decoder(encoded);

                std::vector<uint8_t> decoded(meta.preprocessed_block_size);

                for (size_t si = 0; si < meta.preprocessed_block_size; ++si) {
                    uint32_t lo, hi, total;
                    int sym;

                    if (ctx_model.has_order2_context()) {
                        uint32_t cum = decoder.get_current_count(ctx_model.get_order2_total());
                        sym = ctx_model.find_symbol_and_get_range_o2(cum, lo, hi, total);
                        if (sym < 0) throw std::runtime_error("Symbol not found during O2 decompression");
                        decoder.decode_symbol(lo, hi, total);
                        if (sym == 256) {
                            if (ctx_model.has_order1_context()) {
                                const uint32_t* o2_freq = ctx_model.get_order2_freq_ptr();
                                uint32_t o1_total = ctx_model.get_order1_total_excl_ctx(o2_freq);
                                cum = decoder.get_current_count(o1_total);
                                sym = ctx_model.find_symbol_and_get_range_order1_excl_ctx(cum, o2_freq, o1_total, lo, hi, total);
                                if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                                decoder.decode_symbol(lo, hi, total);
                                if (sym == 256) {
                                    const uint32_t* ctx_freq = ctx_model.get_order1_freq_ptr();
                                    uint32_t excl_total = ctx_model.get_order0_total_excl_ctx(ctx_freq);
                                    cum = decoder.get_current_count(excl_total);
                                    sym = ctx_model.find_symbol_and_get_range_excl_ctx(cum, ctx_freq, excl_total, lo, hi, total);
                                    if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                                    decoder.decode_symbol(lo, hi, total);
                                }
                            } else {
                                cum = decoder.get_current_count(ctx_model.get_order0_total());
                                sym = ctx_model.find_symbol_and_get_range(0, cum, lo, hi, total);
                                if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                                decoder.decode_symbol(lo, hi, total);
                            }
                        }
                    } else if (ctx_model.has_order1_context()) {
                        uint32_t cum = decoder.get_current_count(ctx_model.get_order1_total());
                        sym = ctx_model.find_symbol_and_get_range(1, cum, lo, hi, total);
                        if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                        decoder.decode_symbol(lo, hi, total);
                        if (sym == 256) {
                            // Escape: fall back to order-0 with Method C exclusions
                            const uint32_t* ctx_freq = ctx_model.get_order1_freq_ptr();
                            uint32_t excl_total = ctx_model.get_order0_total_excl_ctx(ctx_freq);
                            cum = decoder.get_current_count(excl_total);
                            sym = ctx_model.find_symbol_and_get_range_excl_ctx(cum, ctx_freq, excl_total, lo, hi, total);
                            if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                            decoder.decode_symbol(lo, hi, total);
                        }
                    } else {
                        uint32_t cum = decoder.get_current_count(ctx_model.get_order0_total());
                        sym = ctx_model.find_symbol_and_get_range(0, cum, lo, hi, total);
                        if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                        decoder.decode_symbol(lo, hi, total);
                    }

                    uint8_t b = static_cast<uint8_t>(sym);
                    decoded[si] = b;
                    ctx_model.update_frequencies(b);
                    ctx_model.update_history(b);
                }

                if (StreamHeader::has_flag(meta.transform_flags, StreamHeader::TRANSFORM_ZRLE)) {
                    decoded = ZeroRunLengthEncoder::decode(decoded);
                }
                if (StreamHeader::has_flag(meta.transform_flags, StreamHeader::TRANSFORM_MTF)) {
                    decoded = MoveToFront::inverse_transform(decoded);
                }
                if (StreamHeader::has_flag(meta.transform_flags, StreamHeader::TRANSFORM_BWT)) {
                    decoded = BWT::inverse_transform(decoded, meta.bwt_primary_index);
                }
                if (StreamHeader::has_flag(meta.transform_flags, StreamHeader::TRANSFORM_X86)) {
                    decoded = X86Filter::decode(decoded, block_base_offset);
                }
                if (StreamHeader::has_flag(meta.transform_flags, StreamHeader::TRANSFORM_LZP)) {
                    decoded = LZPredictor::decode(decoded, meta.original_block_size);
                }

                block_outputs[bi] = std::move(decoded);
            };

            std::vector<std::thread> threads;
            threads.reserve(num_blocks);
            for (size_t i = 0; i < num_blocks; i++) {
                threads.emplace_back(decompress_one_block, i);
            }
            for (auto& t : threads) t.join();

            std::vector<uint8_t> output_data;
            output_data.reserve(ph.original_size);
            for (const auto& bo : block_outputs) {
                output_data.insert(output_data.end(), bo.begin(), bo.end());
            }

            if (output_data.size() != ph.original_size) {
                std::cerr << "Warning: Decoded size (" << output_data.size()
                          << ") doesn't match expected size (" << ph.original_size << ")" << std::endl;
            }

            std::cout << "Writing decompressed file: " << output_filename << std::endl;
            FileIO::write_file(output_filename, output_data);

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            std::cout << "\n=== Decompression Statistics ===" << std::endl;
            std::cout << "Compressed size:    " << compressed_data.size() << " bytes" << std::endl;
            std::cout << "Decompressed size:  " << output_data.size() << " bytes" << std::endl;
            std::cout << "Decompression time: " << duration.count() << " ms" << std::endl;

            return 0;
        }

        StreamHeader::Header header = StreamHeader::parse_header(compressed_data);
        ModelType model_type = header.model_type;
        std::string model_name = StreamHeader::describe_model_type(header);
        std::cout << "Model type: " << model_name << std::endl;
        std::cout << "Original size: " << header.original_size << " bytes" << std::endl;
        if (header.uses_bwt()) {
            std::cout << "BWT block count: " << header.bwt_primary_indices.size() << std::endl;
        }
        if (model_type == ModelType::ORDER_0_PREPROC || model_type == ModelType::ORDER_1_PREPROC) {
            std::cout << "Preprocessed size: " << header.preprocessed_size << " bytes" << std::endl;
        }

        std::vector<uint8_t> output_data;
        output_data.reserve(header.preprocessed_size);

        if (header.is_order0()) {
            if (compressed_data.size() < header.payload_offset + 257 * 4) {
                throw std::runtime_error("Invalid Order-0 compressed file: header too small");
            }

            std::array<uint32_t, 257> frequencies;
            size_t offset = header.payload_offset;
            for (int i = 0; i < 257; i++) {
                uint32_t freq = 0;
                for (int j = 0; j < 4; j++) {
                    freq |= (uint32_t)compressed_data[offset++] << (j * 8);
                }
                frequencies[i] = freq;
            }

            FrequencyModel model;
            model.set_frequencies(frequencies);

            std::vector<uint8_t> encoded_data(compressed_data.begin() + offset, compressed_data.end());

            std::cout << "Decoding with Order-0 model..." << std::endl;
            RangeDecoder decoder(encoded_data);

            const uint32_t total_freq = model.get_total_freq();
            while (output_data.size() < header.preprocessed_size) {
                uint32_t cum_freq = decoder.get_current_count(total_freq);
                int symbol = model.find_symbol(cum_freq);

                if (symbol == FrequencyModel::get_eof_symbol()) {
                    break;
                }

                output_data.push_back(static_cast<uint8_t>(symbol));

                uint32_t cum_freq_low, cum_freq_high;
                model.get_symbol_range(symbol, cum_freq_low, cum_freq_high);
                decoder.decode_symbol(cum_freq_low, cum_freq_high, total_freq);
            }

        } else if (header.is_order1()) {
            std::string model_name_simple = "Order-1 (Simple)";
            if (model_type == ModelType::ORDER_2) model_name_simple = "Order-2";
            std::cout << "Decoding with adaptive " << model_name_simple << " context model..." << std::endl;

            ContextModel model;
            model.set_encoding_method_simple();
            if (model_type == ModelType::ORDER_2) {
                model.enable_order2();
            }
            model.init_adaptive();

            std::vector<uint8_t> encoded_data(compressed_data.begin() + header.payload_offset, compressed_data.end());
            RangeDecoder decoder(encoded_data);

            output_data.resize(header.preprocessed_size);

            for (size_t si = 0; si < header.preprocessed_size; ++si) {
                uint32_t lo, hi, total;
                int sym;

                if (model.has_order2_context()) {
                    uint32_t cum = decoder.get_current_count(model.get_order2_total());
                    sym = model.find_symbol_and_get_range_o2(cum, lo, hi, total);
                    if (sym < 0) throw std::runtime_error("Symbol not found during O2 decompression");
                    decoder.decode_symbol(lo, hi, total);
                    if (sym == 256) {
                        if (model.has_order1_context()) {
                            const uint32_t* o2_freq = model.get_order2_freq_ptr();
                            uint32_t o1_total = model.get_order1_total_excl_ctx(o2_freq);
                            cum = decoder.get_current_count(o1_total);
                            sym = model.find_symbol_and_get_range_order1_excl_ctx(cum, o2_freq, o1_total, lo, hi, total);
                            if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                            decoder.decode_symbol(lo, hi, total);
                            if (sym == 256) {
                                const uint32_t* ctx_freq = model.get_order1_freq_ptr();
                                uint32_t excl_total = model.get_order0_total_excl_ctx(ctx_freq);
                                cum = decoder.get_current_count(excl_total);
                                sym = model.find_symbol_and_get_range_excl_ctx(cum, ctx_freq, excl_total, lo, hi, total);
                                if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                                decoder.decode_symbol(lo, hi, total);
                            }
                        } else {
                            cum = decoder.get_current_count(model.get_order0_total());
                            sym = model.find_symbol_and_get_range(0, cum, lo, hi, total);
                            if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                            decoder.decode_symbol(lo, hi, total);
                        }
                    }
                } else if (model.has_order1_context()) {
                    uint32_t cum = decoder.get_current_count(model.get_order1_total());
                    sym = model.find_symbol_and_get_range(1, cum, lo, hi, total);
                    if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                    decoder.decode_symbol(lo, hi, total);
                    if (sym == 256) {
                        const uint32_t* ctx_freq = model.get_order1_freq_ptr();
                        uint32_t excl_total = model.get_order0_total_excl_ctx(ctx_freq);
                        cum = decoder.get_current_count(excl_total);
                        sym = model.find_symbol_and_get_range_excl_ctx(cum, ctx_freq, excl_total, lo, hi, total);
                        if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                        decoder.decode_symbol(lo, hi, total);
                    }
                } else {
                    uint32_t cum = decoder.get_current_count(model.get_order0_total());
                    sym = model.find_symbol_and_get_range(0, cum, lo, hi, total);
                    if (sym < 0) throw std::runtime_error("Symbol not found during decompression");
                    decoder.decode_symbol(lo, hi, total);
                }

                uint8_t b = static_cast<uint8_t>(sym);
                output_data[si] = b;
                model.update_frequencies(b);
                model.update_history(b);
            }
        } else if (model_type == ModelType::UNCOMPRESSED) {
            std::cout << "Copying uncompressed data..." << std::endl;
            output_data.insert(output_data.end(),
                             compressed_data.begin() + header.payload_offset,
                             compressed_data.end());
        }

        if (header.uses_zrle()) {
            std::cout << "Reversing zero-run RLE..." << std::endl;
            output_data = ZeroRunLengthEncoder::decode(output_data);
        }

        if (header.uses_mtf()) {
            if (output_data.size() != header.original_size) {
                throw std::runtime_error("Invalid preprocessed stream size before inverse MTF");
            }

            std::cout << "Reversing Move-to-Front transform..." << std::endl;
            output_data = MoveToFront::inverse_transform_blocks(output_data, 1024*1024);
        }

        if (header.uses_bwt()) {
            std::cout << "Applying inverse BWT..." << std::endl;
            auto start_inv_bwt = std::chrono::high_resolution_clock::now();

            output_data = BWT::inverse_transform_blocks(output_data, header.bwt_primary_indices, 1024*1024);

            auto end_inv_bwt = std::chrono::high_resolution_clock::now();
            auto inv_bwt_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_inv_bwt - start_inv_bwt).count();
            std::cout << "Inverse BWT complete (" << inv_bwt_time << " ms)" << std::endl;
        }

        if (StreamHeader::has_flag(header.transform_flags, StreamHeader::TRANSFORM_X86)) {
            std::cout << "Reversing x86 filter..." << std::endl;
            output_data = X86Filter::decode(output_data);
        }

        if (StreamHeader::has_flag(header.transform_flags, StreamHeader::TRANSFORM_LZP)) {
            std::cout << "Reversing LZP preprocessing..." << std::endl;
            output_data = LZPredictor::decode(output_data, header.original_size);
        }

        if (output_data.size() != header.original_size) {
            std::cerr << "Warning: Decoded size (" << output_data.size()
                      << ") doesn't match expected size (" << header.original_size << ")" << std::endl;
        }

        std::cout << "Writing decompressed file: " << output_filename << std::endl;
        FileIO::write_file(output_filename, output_data);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // Statistics
        std::cout << "\n=== Decompression Statistics ===" << std::endl;
        std::cout << "Compressed size:    " << compressed_data.size() << " bytes" << std::endl;
        std::cout << "Decompressed size:  " << output_data.size() << " bytes" << std::endl;
        std::cout << "Decompression time: " << duration.count() << " ms" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
