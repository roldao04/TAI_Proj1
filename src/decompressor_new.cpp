/**
 * Decompressor v7.0 - Bit-Level PPM Decompressor
 *
 * Decompresses files created by compressor_v7
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>

// Utilities and preprocessing
#include "utils/file_io.h"
#include "transform/bwt.h"
#include "transform/mtf.h"
#include "transform/zero_rle.h"

// v7 components
#include "model/bit_ppm.h"
#include "model/context_mixer.h"
#include "arithmetic/bit_arithmetic_coder.h"

struct V7Header {
    uint8_t model_type;          // 0x0B for v7.0
    uint64_t original_size;      // Original file size
    uint64_t processed_size;     // Size after BWT/MTF/ZRLE preprocessing
    uint8_t config_byte;         // BWT/MTF/ZRLE flags
    uint32_t bwt_primary_index;  // BWT primary index (if BWT enabled)
    uint8_t num_bit_orders;      // Number of bit orders
    std::vector<int> bit_orders; // Bit order values
    bool use_bwt;
    bool use_mtf;
    bool use_zrle;
};

V7Header parse_v7_header(const std::vector<uint8_t>& data, size_t& offset) {
    V7Header header;

    if (data.size() < 12) {
        throw std::runtime_error("File too small to contain valid header");
    }

    // Model type
    header.model_type = data[offset++];

    if (header.model_type != 0x0B) {
        throw std::runtime_error("Not a v7.0 compressed file (model type: 0x" +
                                std::to_string((int)header.model_type) +
                                "). Expected 0x0B for bit-level PPM.");
    }

    // Original size (8 bytes, little-endian)
    header.original_size = 0;
    for (int i = 0; i < 8; i++) {
        header.original_size |= ((uint64_t)data[offset++]) << (i * 8);
    }

    // Processed size (8 bytes, little-endian)
    header.processed_size = 0;
    for (int i = 0; i < 8; i++) {
        header.processed_size |= ((uint64_t)data[offset++]) << (i * 8);
    }

    // Config byte
    header.config_byte = data[offset++];
    header.use_bwt = (header.config_byte & 0x01) != 0;
    header.use_mtf = (header.config_byte & 0x02) != 0;
    header.use_zrle = (header.config_byte & 0x04) != 0;

    // BWT primary index (4 bytes, little-endian) - only if BWT enabled
    header.bwt_primary_index = 0;
    if (header.use_bwt) {
        for (int i = 0; i < 4; i++) {
            header.bwt_primary_index |= ((uint32_t)data[offset++]) << (i * 8);
        }
    }

    // Number of bit orders
    header.num_bit_orders = data[offset++];

    // Bit order values
    header.bit_orders.reserve(header.num_bit_orders);
    for (int i = 0; i < header.num_bit_orders; i++) {
        header.bit_orders.push_back(data[offset++]);
    }

    // No mixer weights in header for v7 adaptive mode
    // Both encoder and decoder start with equal weights and adapt identically

    return header;
}

std::vector<uint8_t> decode_with_bit_ppm(const std::vector<uint8_t>& encoded,
                                          size_t offset,
                                          const V7Header& header,
                                          bool verbose) {
    if (header.processed_size == 0) {
        return std::vector<uint8_t>();
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Initialize bit-level PPM (same config as encoder)
    // Use 32M contexts (matches compressor)
    BitLevelPPM bit_ppm(header.bit_orders, 32 * 1024 * 1024, 256);

    // Initialize context mixer (same config as encoder)
    ContextMixer mixer(header.bit_orders.size(), 0.003);
    // Weights start at 1.0 (equal) and will adapt identically to encoder

    if (verbose) {
        std::cout << "Initialized bit-level PPM with adaptive mixing\n";
        std::cout << "Bit orders: ";
        for (int order : header.bit_orders) std::cout << order << " ";
        std::cout << "\n";
    }

    // Initialize bit arithmetic decoder
    std::vector<uint8_t> encoded_data(encoded.begin() + offset, encoded.end());
    BitArithmeticDecoder decoder(encoded_data);

    std::vector<uint8_t> decoded;
    decoded.reserve(header.processed_size);

    if (verbose) {
        std::cout << "\nDecoding with Bit-Level PPM + Adaptive Mixing...\n";
        std::cout << "Target size: " << header.processed_size << " bytes ("
                  << (header.processed_size * 8) << " bits)\n";
    }

    // Decode each byte as 8 bits (MSB to LSB)
    size_t progress_interval = header.processed_size / 100;
    if (progress_interval == 0) progress_interval = 1;

    for (size_t i = 0; i < header.processed_size; i++) {
        uint8_t byte = 0;

        // Process bits from MSB (bit 7) to LSB (bit 0)
        for (int bit_pos = 7; bit_pos >= 0; bit_pos--) {
            // 1. Get predictions from all bit orders
            //    CRITICAL: Must be identical to encoder at this point
            std::vector<Prediction> predictions = bit_ppm.predict_all();

            // 2. Mix predictions (identical to encoder)
            double p_bit1 = mixer.mix(predictions);
            double p_bit0 = 1.0 - p_bit1;

            // 3. Decode bit using arithmetic coder
            bool bit = decoder.decode_bit(p_bit0);

            // Set bit in byte
            if (bit) {
                byte |= (1 << bit_pos);
            }

            // 4. Update bit-PPM models (identical to encoder)
            bit_ppm.update_all(bit);

            // 5. Update mixer weights (identical to encoder)
            mixer.update(predictions, bit);
        }

        decoded.push_back(byte);

        // Progress indicator
        if (verbose && i % progress_interval == 0) {
            int percent = (int)((i * 100) / header.processed_size);
            std::cout << "\rProgress: " << percent << "%" << std::flush;
        }
    }

    if (verbose) {
        std::cout << "\rProgress: 100%\n";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "Decoding time: " << duration.count() << " ms\n";
        std::cout << "Decoded " << decoded.size() << " bytes (processed data)\n";

        // Print final mixer weights
        std::cout << "\nFinal adaptive mixer weights:\n";
        const auto& weights = mixer.get_weights();
        for (size_t i = 0; i < weights.size(); i++) {
            std::cout << "  Bit-Order-" << header.bit_orders[i] << ": " << weights[i] << "\n";
        }
    }

    // CRITICAL VALIDATION: Must decode exactly processed_size bytes
    if (decoded.size() != header.processed_size) {
        throw std::runtime_error(
            "Bit-PPM decode size mismatch: got " + std::to_string(decoded.size()) +
            " bytes, expected " + std::to_string(header.processed_size) + " bytes"
        );
    }

    return decoded;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <compressed_file> <output_file> [--quiet]\n\n";
        std::cout << "Decompresses files created by compress_v7\n";
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];
    bool verbose = true;

    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--quiet") {
            verbose = false;
        }
    }

    if (verbose) {
        std::cout << "╔══════════════════════════════════════════╗\n";
        std::cout << "║  Lossless Decompression Tool v7.0       ║\n";
        std::cout << "║  Bit-Level PPM Decompressor             ║\n";
        std::cout << "║  Group 07 - Universidade de Aveiro     ║\n";
        std::cout << "╚══════════════════════════════════════════╝\n\n";
    }

    // Read compressed file
    std::cout << "Reading compressed file: " << input_file << "\n";
    std::vector<uint8_t> compressed_data;
    try {
        compressed_data = FileIO::read_file(input_file);
    } catch (const std::exception& e) {
        std::cerr << "Error reading compressed file: " << e.what() << std::endl;
        return 1;
    }

    if (compressed_data.empty()) {
        std::cerr << "Error: Compressed file is empty\n";
        return 1;
    }

    std::cout << "Compressed size: " << compressed_data.size() << " bytes\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    // Parse header
    size_t offset = 0;
    V7Header header;
    try {
        header = parse_v7_header(compressed_data, offset);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing header: " << e.what() << std::endl;
        return 1;
    }

    if (verbose) {
        std::cout << "Original size: " << header.original_size << " bytes\n";
        std::cout << "Configuration:\n";
        std::cout << "  BWT: " << (header.use_bwt ? "enabled" : "disabled");
        if (header.use_bwt) {
            std::cout << " (primary index: " << header.bwt_primary_index << ")";
        }
        std::cout << "\n";
        std::cout << "  MTF: " << (header.use_mtf ? "enabled" : "disabled") << "\n";
        std::cout << "  ZRLE: " << (header.use_zrle ? "enabled" : "disabled") << "\n";
    }

    // Decode
    std::vector<uint8_t> decoded_data = decode_with_bit_ppm(compressed_data, offset, header, verbose);

    if (verbose) {
        std::cout << "\nDecoded " << decoded_data.size() << " bytes (processed data)\n";
    }

    // Reverse preprocessing (reverse order of compression)
    if (header.use_zrle && header.use_mtf) {
        std::cout << "\nReversing zero-run RLE...\n";
        std::cout << "  Input to ZRLE decode: " << decoded_data.size() << " bytes\n";

        size_t before_zrle = decoded_data.size();
        decoded_data = ZeroRunLengthEncoder::decode(decoded_data);

        std::cout << "  ZRLE output: " << decoded_data.size() << " bytes\n";
        std::cout << "  ZRLE reversed: " << before_zrle << " → " << decoded_data.size() << " bytes\n";

        if (decoded_data.size() != header.original_size) {
            throw std::runtime_error(
                "ZRLE decode size mismatch: got " + std::to_string(decoded_data.size()) +
                " bytes, expected " + std::to_string(header.original_size) + " bytes"
            );
        }
    }

    if (header.use_mtf && header.use_bwt) {
        std::cout << "\nReversing MTF transform...\n";
        size_t before_mtf = decoded_data.size();
        decoded_data = MoveToFront::inverse_transform(decoded_data);
        std::cout << "  MTF reversed: " << before_mtf << " → " << decoded_data.size() << " bytes\n";

        if (decoded_data.size() != before_mtf) {
            throw std::runtime_error("MTF inverse changed data size!");
        }
    }

    if (header.use_bwt) {
        std::cout << "\nReversing BWT transform...\n";
        std::cout << "  Input size: " << decoded_data.size() << " bytes\n";
        std::cout << "  Primary index: " << header.bwt_primary_index << "\n";
        std::cout << "  Expected size: " << header.original_size << " bytes\n";

        decoded_data = BWT::inverse_transform(decoded_data, header.bwt_primary_index);
        std::cout << "  BWT reversed. Size: " << decoded_data.size() << " bytes\n";
    }

    // Write output file
    try {
        FileIO::write_file(output_file, decoded_data);
    } catch (const std::exception& e) {
        std::cerr << "Error writing output file: " << e.what() << std::endl;
        return 1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Print results
    if (verbose) {
        std::cout << "\n╔══════════════════════════════════════════╗\n";
        std::cout << "║         Decompression Complete!         ║\n";
        std::cout << "╚══════════════════════════════════════════╝\n\n";
        std::cout << "Input file:      " << input_file << "\n";
        std::cout << "Output file:     " << output_file << "\n";
        std::cout << "Compressed size: " << compressed_data.size() << " bytes\n";
        std::cout << "Original size:   " << decoded_data.size() << " bytes\n";
        std::cout << "Decompression time: " << duration.count() << " ms\n";
        std::cout << "Throughput:      " << (decoded_data.size() / 1024.0 / (duration.count() / 1000.0))
                  << " KB/s\n";
    }

    return 0;
}
