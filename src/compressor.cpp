#include <iostream>
#include <chrono>
#include "arithmetic/arithmetic_coder.h"
#include "model/frequency_model.h"
#include "utils/file_io.h"

/*
 * Lossless Data Compressor
 *
 * Uses:
 * - Order-0 frequency model for probability estimation
 * - Arithmetic coding for entropy coding
 *
 * Compressed file format:
 * - Original file size (8 bytes, uint64_t)
 * - Frequency table (257 * 4 bytes = 1028 bytes)
 * - Arithmetic coded data
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

        // Read input file
        std::cout << "Reading input file: " << input_filename << std::endl;
        std::vector<uint8_t> input_data = FileIO::read_file(input_filename);
        std::cout << "Input size: " << input_data.size() << " bytes" << std::endl;

        // Build frequency model
        std::cout << "Building frequency model..." << std::endl;
        FrequencyModel model;
        model.build_from_data(input_data);

        // Encode data
        std::cout << "Encoding..." << std::endl;
        ArithmeticEncoder encoder;

        for (uint8_t byte : input_data) {
            uint32_t cum_freq_low, cum_freq_high;
            model.get_symbol_range(byte, cum_freq_low, cum_freq_high);
            encoder.encode_symbol(cum_freq_low, cum_freq_high, model.get_total_freq());
        }

        // Encode EOF symbol
        uint32_t cum_freq_low, cum_freq_high;
        model.get_symbol_range(FrequencyModel::get_eof_symbol(), cum_freq_low, cum_freq_high);
        encoder.encode_symbol(cum_freq_low, cum_freq_high, model.get_total_freq());

        encoder.finish();
        const std::vector<uint8_t>& encoded_data = encoder.get_output();

        // Prepare output: original size + frequency table + encoded data
        std::vector<uint8_t> output_data;

        // Write original file size (8 bytes)
        uint64_t original_size = input_data.size();
        for (int i = 0; i < 8; i++) {
            output_data.push_back((original_size >> (i * 8)) & 0xFF);
        }

        // Write frequency table (257 * 4 bytes)
        const auto& frequencies = model.get_frequencies();
        for (int i = 0; i < 257; i++) {
            uint32_t freq = frequencies[i];
            for (int j = 0; j < 4; j++) {
                output_data.push_back((freq >> (j * 8)) & 0xFF);
            }
        }

        // Write encoded data
        output_data.insert(output_data.end(), encoded_data.begin(), encoded_data.end());

        // Write output file
        std::cout << "Writing compressed file: " << output_filename << std::endl;
        FileIO::write_file(output_filename, output_data);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // Statistics
        std::cout << "\n=== Compression Statistics ===" << std::endl;
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
