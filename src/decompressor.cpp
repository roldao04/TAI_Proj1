#include <iostream>
#include <chrono>
#include <array>
#include "arithmetic/arithmetic_coder.h"
#include "model/frequency_model.h"
#include "utils/file_io.h"

/*
 * Lossless Data Decompressor
 *
 * Reverses the compression process:
 * - Reads compressed file
 * - Extracts frequency model
 * - Decodes arithmetic coded data
 * - Reconstructs original file
 */

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

        if (compressed_data.size() < 8 + 257 * 4) {
            throw std::runtime_error("Invalid compressed file: too small");
        }

        // Read original file size (8 bytes)
        uint64_t original_size = 0;
        for (int i = 0; i < 8; i++) {
            original_size |= (uint64_t)compressed_data[i] << (i * 8);
        }
        std::cout << "Original size: " << original_size << " bytes" << std::endl;

        // Read frequency table (257 * 4 bytes)
        std::array<uint32_t, 257> frequencies;
        size_t offset = 8;
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
        std::cout << "Decoding..." << std::endl;
        ArithmeticDecoder decoder(encoded_data);
        std::vector<uint8_t> output_data;
        output_data.reserve(original_size);

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
