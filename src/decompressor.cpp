#include <iostream>
#include <chrono>
#include <array>
#include "arithmetic/range_coder.h"
#include "model/frequency_model.h"
#include "model/context_model.h"
#include "utils/file_io.h"

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

        // Read compressed file
        std::cout << "Reading compressed file: " << input_filename << std::endl;
        std::vector<uint8_t> compressed_data = FileIO::read_file(input_filename);
        std::cout << "Compressed size: " << compressed_data.size() << " bytes" << std::endl;

        if (compressed_data.size() < 9) {  // At least model type + original size
            throw std::runtime_error("Invalid compressed file: too small");
        }

        // Read model type (1 byte)
        ModelType model_type = static_cast<ModelType>(compressed_data[0]);
        std::string model_name;
        if (model_type == ModelType::ORDER_0) model_name = "Order-0";
        else if (model_type == ModelType::ORDER_1) model_name = "Order-1";
        else if (model_type == ModelType::ORDER_2) model_name = "Order-2";
        else if (model_type == ModelType::UNCOMPRESSED) model_name = "Uncompressed";
        else model_name = "Unknown";
        std::cout << "Model type: " << model_name << std::endl;

        // Read original file size (8 bytes)
        uint64_t original_size = 0;
        for (int i = 0; i < 8; i++) {
            original_size |= (uint64_t)compressed_data[1 + i] << (i * 8);
        }
        std::cout << "Original size: " << original_size << " bytes" << std::endl;

        std::vector<uint8_t> output_data;
        output_data.reserve(original_size);

        if (model_type == ModelType::ORDER_0) {
            // Order-0: Read frequency table from header
            if (compressed_data.size() < 9 + 257 * 4) {
                throw std::runtime_error("Invalid Order-0 compressed file: header too small");
            }

            // Read frequency table (257 * 4 bytes)
            std::array<uint32_t, 257> frequencies;
            size_t offset = 9;
            for (int i = 0; i < 257; i++) {
                uint32_t freq = 0;
                for (int j = 0; j < 4; j++) {
                    freq |= (uint32_t)compressed_data[offset++] << (j * 8);
                }
                frequencies[i] = freq;
            }

            // Build frequency model
            FrequencyModel model;
            model.set_frequencies(frequencies);

            // Extract encoded data
            std::vector<uint8_t> encoded_data(compressed_data.begin() + offset, compressed_data.end());

            // Decode data
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

        } else if (model_type == ModelType::ORDER_1 || model_type == ModelType::ORDER_2) {
            // Order-1/2 adaptive: No header, build model during decompression
            std::string model_name = (model_type == ModelType::ORDER_1) ? "Order-1 (Simple)" : "Order-2";
            std::cout << "Decoding with adaptive " << model_name << " context model..." << std::endl;

            // Initialize adaptive model in simple mode (MUST match encoder!)
            ContextModel model;
            model.set_encoding_method_simple();  // Use simplified mode (Phase 1)
            model.init_adaptive();

            // Extract encoded data (starts right after size header)
            std::vector<uint8_t> encoded_data(compressed_data.begin() + 9, compressed_data.end());
            RangeDecoder decoder(encoded_data);

            // SRP: Simplified decoding loop (no exclusions, matches encode_symbol_simple)
            while (output_data.size() < original_size) {
                // Determine starting order based on history (match encoder logic!)
                int current_order = std::min(static_cast<int>(model.get_history_size()), 2);

                // Phase 1: Only use Order-1 (skip Order-2)
                if (current_order > 1) {
                    current_order = 1;
                }

                uint8_t decoded_byte = 0;
                bool symbol_found = false;

                // Try decoding from highest to lowest order (simple escape mechanism)
                while (current_order >= 0 && !symbol_found) {
                    uint32_t total_freq = model.get_total_freq(current_order);

                    if (total_freq == 0) {
                        // Context doesn't exist yet, fall back to lower order
                        current_order--;
                        continue;
                    }

                    // Decode symbol from this order
                    uint32_t cum_freq = decoder.get_current_count(total_freq);
                    int symbol = model.find_symbol(current_order, cum_freq);

                    if (symbol < 0) {
                        throw std::runtime_error("Symbol not found during decompression");
                    }

                    if (symbol == 256) {  // ESCAPE_SYMBOL
                        // Decode the escape and move to lower order
                        uint32_t cum_freq_low, cum_freq_high, total;
                        model.get_symbol_range(current_order, symbol, cum_freq_low, cum_freq_high, total);
                        decoder.decode_symbol(cum_freq_low, cum_freq_high, total);

                        // Fall back to lower order (no exclusions tracked)
                        current_order--;
                    } else {
                        // Found actual symbol - decode it
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

                // Update model adaptively (CRITICAL: must match encoder exactly!)
                model.update_frequencies(decoded_byte);
                model.update_history(decoded_byte);
            }
        } else if (model_type == ModelType::UNCOMPRESSED) {
            // Uncompressed: just copy data directly
            std::cout << "Copying uncompressed data..." << std::endl;
            output_data.insert(output_data.end(),
                             compressed_data.begin() + 9,
                             compressed_data.end());
        } else {
            throw std::runtime_error("Unknown model type");
        }

        // Verify size
        if (output_data.size() != original_size) {
            std::cerr << "Warning: Decoded size (" << output_data.size()
                      << ") doesn't match expected size (" << original_size << ")" << std::endl;
        }

        // Write output file
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
