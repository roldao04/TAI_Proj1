#include <iostream>
#include <chrono>
#include <array>
#include "arithmetic/range_coder.h"
#include "model/frequency_model.h"
#include "model/context_model.h"
#include "utils/file_io.h"
#include "transform/bwt.h"

/*
 * Lossless Data Decompressor
 *
 * Supports multiple compression models:
 * - Order-0: Simple frequency model (reads model from header)
 * - Order-2: Adaptive context model (builds model during decompression)
 */

enum class ModelType {
    ORDER_0 = 0,
    ORDER_1 = 1,
    ORDER_2 = 2,
    ORDER_0_BWT = 3,
    ORDER_1_BWT = 4,
    UNCOMPRESSED = 255
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }

    std::string input_filename = argv[1];
    std::string output_filename = argv[2];

    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        std::cout << "Reading compressed file: " << input_filename << std::endl;
        std::vector<uint8_t> compressed_data = FileIO::read_file(input_filename);
        std::cout << "Compressed size: " << compressed_data.size() << " bytes" << std::endl;

        if (compressed_data.size() < 9) {
            throw std::runtime_error("Invalid compressed file: too small");
        }

        ModelType model_type = static_cast<ModelType>(compressed_data[0]);
        std::string model_name;
        bool use_bwt = false;
        if (model_type == ModelType::ORDER_0) model_name = "Order-0";
        else if (model_type == ModelType::ORDER_1) model_name = "Order-1";
        else if (model_type == ModelType::ORDER_2) model_name = "Order-2";
        else if (model_type == ModelType::ORDER_0_BWT) {
            model_name = "Order-0 with BWT";
            use_bwt = true;
        }
        else if (model_type == ModelType::ORDER_1_BWT) {
            model_name = "Order-1 with BWT";
            use_bwt = true;
        }
        else if (model_type == ModelType::UNCOMPRESSED) model_name = "Uncompressed";
        else model_name = "Unknown";
        std::cout << "Model type: " << model_name << std::endl;

        uint64_t original_size = 0;
        for (int i = 0; i < 8; i++) {
            original_size |= (uint64_t)compressed_data[1 + i] << (i * 8);
        }
        std::cout << "Original size: " << original_size << " bytes" << std::endl;

        std::vector<uint32_t> bwt_primary_indices;
        size_t bwt_header_offset = 9;

        if (use_bwt) {
            uint32_t block_count = 0;
            for (int i = 0; i < 4; i++) {
                block_count |= (uint32_t)compressed_data[bwt_header_offset++] << (i * 8);
            }
            std::cout << "BWT block count: " << block_count << std::endl;

            bwt_primary_indices.reserve(block_count);
            for (uint32_t i = 0; i < block_count; i++) {
                uint32_t idx = 0;
                for (int j = 0; j < 4; j++) {
                    idx |= (uint32_t)compressed_data[bwt_header_offset++] << (j * 8);
                }
                bwt_primary_indices.push_back(idx);
            }
        }

        std::vector<uint8_t> output_data;
        output_data.reserve(original_size);

        if (model_type == ModelType::ORDER_0 || model_type == ModelType::ORDER_0_BWT) {
            if (compressed_data.size() < 9 + 257 * 4) {
                throw std::runtime_error("Invalid Order-0 compressed file: header too small");
            }

            std::array<uint32_t, 257> frequencies;
            size_t offset = bwt_header_offset;
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

            while (output_data.size() < original_size) {
                uint32_t cum_freq = decoder.get_current_count(model.get_total_freq());
                int symbol = model.find_symbol(cum_freq);

                if (symbol == FrequencyModel::get_eof_symbol()) {
                    break;
                }

                output_data.push_back(static_cast<uint8_t>(symbol));

                uint32_t cum_freq_low, cum_freq_high;
                model.get_symbol_range(symbol, cum_freq_low, cum_freq_high);
                decoder.decode_symbol(cum_freq_low, cum_freq_high, model.get_total_freq());
            }

        } else if (model_type == ModelType::ORDER_1 || model_type == ModelType::ORDER_2 || model_type == ModelType::ORDER_1_BWT) {
            std::string model_name_simple = "Order-1 (Simple)";
            if (model_type == ModelType::ORDER_2) model_name_simple = "Order-2";
            std::cout << "Decoding with adaptive " << model_name_simple << " context model..." << std::endl;

            ContextModel model;
            model.set_encoding_method_simple();
            model.init_adaptive();

            std::vector<uint8_t> encoded_data(compressed_data.begin() + bwt_header_offset, compressed_data.end());
            RangeDecoder decoder(encoded_data);

            while (output_data.size() < original_size) {
                int current_order = std::min(static_cast<int>(model.get_history_size()), 2);

                if (current_order > 1) {
                    current_order = 1;
                }

                uint8_t decoded_byte = 0;
                bool symbol_found = false;

                while (current_order >= 0 && !symbol_found) {
                    uint32_t total_freq = model.get_total_freq(current_order);

                    if (total_freq == 0) {
                        current_order--;
                        continue;
                    }

                    uint32_t cum_freq = decoder.get_current_count(total_freq);
                    int symbol = model.find_symbol(current_order, cum_freq);

                    if (symbol < 0) {
                        throw std::runtime_error("Symbol not found during decompression");
                    }

                    if (symbol == 256) {
                        uint32_t cum_freq_low, cum_freq_high, total;
                        model.get_symbol_range(current_order, symbol, cum_freq_low, cum_freq_high, total);
                        decoder.decode_symbol(cum_freq_low, cum_freq_high, total);

                        current_order--;
                    } else {
                        uint32_t cum_freq_low, cum_freq_high, total;
                        model.get_symbol_range(current_order, symbol, cum_freq_low, cum_freq_high, total);
                        decoder.decode_symbol(cum_freq_low, cum_freq_high, total);

                        decoded_byte = static_cast<uint8_t>(symbol);
                        symbol_found = true;
                    }
                }

                if (!symbol_found) {
                    throw std::runtime_error("Failed to decode symbol");
                }

                output_data.push_back(decoded_byte);

                model.update_frequencies(decoded_byte);
                model.update_history(decoded_byte);
            }
        } else if (model_type == ModelType::UNCOMPRESSED) {
            std::cout << "Copying uncompressed data..." << std::endl;
            output_data.insert(output_data.end(),
                             compressed_data.begin() + 9,
                             compressed_data.end());
        } else {
            throw std::runtime_error("Unknown model type");
        }

        if (use_bwt) {
            std::cout << "Applying inverse BWT..." << std::endl;
            auto start_inv_bwt = std::chrono::high_resolution_clock::now();

            output_data = BWT::inverse_transform_blocks(output_data, bwt_primary_indices, 900*1024);

            auto end_inv_bwt = std::chrono::high_resolution_clock::now();
            auto inv_bwt_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_inv_bwt - start_inv_bwt).count();
            std::cout << "Inverse BWT complete (" << inv_bwt_time << " ms)" << std::endl;
        }

        if (output_data.size() != original_size) {
            std::cerr << "Warning: Decoded size (" << output_data.size()
                      << ") doesn't match expected size (" << original_size << ")" << std::endl;
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
