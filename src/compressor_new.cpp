/**
 * Compressor v7.0 - Bit-Level PPM with Adaptive Mixing
 *
 * Focus: Maximum compression with proper adaptive learning
 * Features:
 * - Bit-level PPM: Contexts = last N bits (not bytes)
 * - Bit orders: {4, 8, 12, 16, 24} bits
 * - Adaptive context mixing (weights update per bit)
 * - Bit arithmetic coder
 * - BWT/MTF/ZRLE preprocessing (from v5.0/v6.0)
 *
 * Trade-offs:
 * - 50-200× slower than v5.0
 * - Uses 0.5-2 GB RAM
 * - Target: 48-52% compression ratio
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>

// Utilities and preprocessing (reuse from v5/v6)
#include "utils/file_io.h"
#include "utils/entropy_calculator.h"
#include "transform/bwt.h"
#include "transform/mtf.h"
#include "transform/zero_rle.h"

// v7 components (bit-level)
#include "model/bit_ppm.h"
#include "model/context_mixer.h"
#include "arithmetic/bit_arithmetic_coder.h"

// Configuration
struct V7Config {
    // Optimized bit orders for balance between coverage and learning speed
    // Order-2: Very short patterns (2 bits)
    // Order-4,8: Short contexts (0.5-1 byte) - most important
    // Order-12,16: Medium contexts (1.5-2 bytes)
    // Order-24: Long context (3 bytes)
    std::vector<int> bit_orders = {2, 4, 8, 12, 16, 24};
    size_t hash_table_size = 32 * 1024 * 1024;           // 32M contexts (~400MB RAM) - balanced
    double learning_rate = 0.003;                        // Slightly higher for faster adaptation
    bool use_bwt = true;                                 // Enable BWT preprocessing
    bool use_mtf = true;                                 // Enable MTF transform
    bool use_zrle = true;                                // Enable zero-run RLE
    bool verbose = true;                                 // Print statistics
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <input_file> <output_file> [options]\n\n";
    std::cout << "v7.0 Options (Bit-Level PPM - Enhanced):\n";
    std::cout << "  --bit-orders <list>   Comma-separated bit orders (default: 1,2,4,6,8,12,16,20,24,32)\n";
    std::cout << "  --hash-size <M>       Hash table size in millions (default: 64)\n";
    std::cout << "  --learning-rate <lr>  Mixer learning rate (default: 0.002)\n";
    std::cout << "  --no-bwt              Disable BWT preprocessing\n";
    std::cout << "  --no-mtf              Disable MTF transform\n";
    std::cout << "  --no-zrle             Disable zero-run RLE\n";
    std::cout << "  --quiet               Minimal output\n";
    std::cout << "  --yes, -y             Skip prompts\n";
    std::cout << "\nNote: v7.0 uses PAQ-style bit-level prediction with 10 context orders\n";
}

/**
 * Encode data using bit-level PPM with adaptive context mixing
 */
std::vector<uint8_t> encode_with_bit_ppm(const std::vector<uint8_t>& data,
                                          const V7Config& config) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Initialize bit-level PPM
    BitLevelPPM bit_ppm(config.bit_orders, config.hash_table_size, 256);

    // Initialize context mixer with equal weights (will adapt during encoding)
    ContextMixer mixer(config.bit_orders.size(), config.learning_rate);

    // Initialize bit arithmetic encoder
    std::vector<uint8_t> output;
    BitArithmeticEncoder encoder(output);

    if (config.verbose) {
        std::cout << "\nEncoding with Bit-Level PPM + Adaptive Mixing...\n";
        std::cout << "Bit orders: ";
        for (int order : config.bit_orders) std::cout << order << " ";
        std::cout << "\nData size: " << data.size() << " bytes (" << (data.size() * 8) << " bits)\n";
    }

    // Process each byte as 8 bits (MSB to LSB)
    size_t total_bits = data.size() * 8;
    size_t progress_interval = data.size() / 100;
    if (progress_interval == 0) progress_interval = 1;

    for (size_t i = 0; i < data.size(); i++) {
        uint8_t byte = data[i];

        // Process bits from MSB (bit 7) to LSB (bit 0)
        for (int bit_pos = 7; bit_pos >= 0; bit_pos--) {
            bool bit = (byte >> bit_pos) & 1;

            // 1. Get predictions from all bit orders
            std::vector<Prediction> predictions = bit_ppm.predict_all();

            // 2. Mix predictions using context mixer
            double p_bit1 = mixer.mix(predictions);
            double p_bit0 = 1.0 - p_bit1;

            // 3. Encode bit using arithmetic coder
            encoder.encode_bit(bit, p_bit0);

            // 4. Update bit-PPM models
            bit_ppm.update_all(bit);

            // 5. Update mixer weights (ADAPTIVE LEARNING)
            mixer.update(predictions, bit);
        }

        // Progress indicator
        if (config.verbose && i % progress_interval == 0) {
            int percent = (int)((i * 100) / data.size());
            std::cout << "\rProgress: " << percent << "%" << std::flush;
        }
    }

    // Finish encoding
    encoder.finish();

    if (config.verbose) {
        std::cout << "\rProgress: 100%\n";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "Encoding time: " << duration.count() << " ms\n";
        std::cout << "Encoded size: " << output.size() << " bytes\n";
        std::cout << "Compression ratio: " << (output.size() * 100.0 / data.size()) << "%\n";

        // Print final mixer weights
        std::cout << "\nFinal adaptive mixer weights:\n";
        const auto& weights = mixer.get_weights();
        for (size_t i = 0; i < weights.size(); i++) {
            std::cout << "  Bit-Order-" << config.bit_orders[i] << ": " << weights[i] << "\n";
        }

        // Print PPM statistics
        std::cout << "\nBit-PPM hash table usage: "
                  << bit_ppm.get_hash_table_size() << " / "
                  << bit_ppm.get_hash_table_limit() << " contexts ("
                  << (bit_ppm.get_hash_table_size() * 100.0 / bit_ppm.get_hash_table_limit()) << "%)\n";
        std::cout << "Memory usage: " << (bit_ppm.get_memory_usage() / (1024*1024)) << " MB\n";
    }

    return output;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];

    // Parse configuration
    V7Config config;
    bool skip_prompts = false;

    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--no-bwt") {
            config.use_bwt = false;
        } else if (arg == "--no-mtf") {
            config.use_mtf = false;
        } else if (arg == "--no-zrle") {
            config.use_zrle = false;
        } else if (arg == "--quiet") {
            config.verbose = false;
        } else if (arg == "--yes" || arg == "-y") {
            skip_prompts = true;
        } else if (arg == "--hash-size" && i + 1 < argc) {
            config.hash_table_size = std::stoull(argv[++i]) * 1024 * 1024;
        } else if (arg == "--learning-rate" && i + 1 < argc) {
            config.learning_rate = std::stod(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
    }

    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║  Lossless Compression Tool v7.0         ║\n";
    std::cout << "║  Bit-Level PPM with Adaptive Mixing     ║\n";
    std::cout << "║  Group 07 - Universidade de Aveiro     ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";

    // Read input file
    std::cout << "Reading input file: " << input_file << "\n";
    std::vector<uint8_t> input_data;
    try {
        input_data = FileIO::read_file(input_file);
    } catch (const std::exception& e) {
        std::cerr << "Error reading input file: " << e.what() << std::endl;
        return 1;
    }

    if (input_data.empty()) {
        std::cerr << "Error: Input file is empty\n";
        return 1;
    }

    std::cout << "Input size: " << input_data.size() << " bytes\n";

    // Calculate entropy
    double entropy = EntropyCalculator::calculate(input_data);
    std::cout << "Entropy: " << entropy << " bits/symbol\n";

    std::cout << "BWT preprocessing: " << (config.use_bwt ? "ENABLED" : "DISABLED") << "\n";
    std::cout << "MTF transform: " << (config.use_mtf ? "ENABLED" : "DISABLED") << "\n";
    std::cout << "ZRLE: " << (config.use_zrle ? "ENABLED" : "DISABLED") << "\n";

    // Warning about speed
    if (!skip_prompts) {
        std::cout << "\n⚠️  Note: v7.0 is slower than v5.0 (50-200× compression time)\n";
        std::cout << "   Estimated time for " << (input_data.size() / 1024) << " KB: "
                  << ((input_data.size() / 1024) * 0.2) << " - "
                  << ((input_data.size() / 1024) * 2.0) << " seconds\n";
        std::cout << "   Continue? (y/n): ";
        std::string response;
        std::getline(std::cin, response);
        if (response != "y" && response != "Y" && response != "yes") {
            std::cout << "Compression cancelled.\n";
            return 0;
        }
    }

    // Start compression
    auto start_time = std::chrono::high_resolution_clock::now();

    // Preprocessing pipeline (same as v6)
    std::vector<uint8_t> processed_data = input_data;
    uint32_t bwt_primary_index = 0;
    bool bwt_applied = false;
    bool mtf_applied = false;

    if (config.use_bwt) {
        std::cout << "\n[1/4] Applying BWT preprocessing...\n";
        auto bwt_result = BWT::transform(processed_data);
        processed_data = bwt_result.first;
        bwt_primary_index = bwt_result.second;
        bwt_applied = true;
        std::cout << "  BWT applied. Primary index: " << bwt_primary_index << "\n";
    }

    if (config.use_mtf && bwt_applied) {
        std::cout << "\n[2/4] Applying MTF transform...\n";
        processed_data = MoveToFront::transform(processed_data);
        mtf_applied = true;
        std::cout << "  MTF applied. Size: " << processed_data.size() << " bytes\n";
    }

    // Auto-selection for ZRLE
    if (mtf_applied) {
        bool has_rank255 = std::any_of(processed_data.begin(), processed_data.end(),
                                       [](uint8_t b){ return b == 255; });

        if (has_rank255) {
            std::cout << "\n[3/4] Zero-run RLE: SKIPPED (MTF output contains rank-255)\n";
            config.use_zrle = false;
        } else {
            std::vector<uint8_t> zrle_candidate = ZeroRunLengthEncoder::encode(processed_data);
            if (zrle_candidate.size() < processed_data.size()) {
                std::cout << "\n[3/4] Applying zero-run RLE...\n";
                std::cout << "  ZRLE applied: " << processed_data.size() << " -> "
                          << zrle_candidate.size() << " bytes (saved "
                          << (processed_data.size() - zrle_candidate.size()) << " bytes)\n";
                processed_data = std::move(zrle_candidate);
                config.use_zrle = true;
            } else {
                std::cout << "\n[3/4] Zero-run RLE: SKIPPED (no size reduction)\n";
                config.use_zrle = false;
            }
        }
    }

    // Encode with bit-level PPM
    std::cout << "\n[4/4] Encoding with Bit-Level PPM...\n";
    std::vector<uint8_t> encoded_data = encode_with_bit_ppm(processed_data, config);

    // Write output file with header
    std::vector<uint8_t> output_data;

    // Header: model type (0x0B = BIT_LEVEL_PPM_V7)
    output_data.push_back(0x0B);

    // Original size (8 bytes, little-endian)
    uint64_t original_size = input_data.size();
    for (int i = 0; i < 8; i++) {
        output_data.push_back((uint8_t)(original_size >> (i * 8)));
    }

    // Processed size (8 bytes, little-endian)
    uint64_t processed_size = processed_data.size();
    for (int i = 0; i < 8; i++) {
        output_data.push_back((uint8_t)(processed_size >> (i * 8)));
    }

    // Config byte
    uint8_t config_byte = 0;
    if (bwt_applied) config_byte |= 0x01;
    if (mtf_applied) config_byte |= 0x02;
    if (config.use_zrle && mtf_applied) config_byte |= 0x04;
    output_data.push_back(config_byte);

    // BWT primary index (4 bytes, little-endian) - only if BWT applied
    if (bwt_applied) {
        for (int i = 0; i < 4; i++) {
            output_data.push_back((uint8_t)(bwt_primary_index >> (i * 8)));
        }
    }

    // Number of bit orders
    output_data.push_back((uint8_t)config.bit_orders.size());

    // Bit order values
    for (int order : config.bit_orders) {
        output_data.push_back((uint8_t)order);
    }

    // No mixer weights stored - both encoder and decoder start with equal weights
    // and update adaptively using identical learning algorithm

    // Encoded data
    output_data.insert(output_data.end(), encoded_data.begin(), encoded_data.end());

    // Write output file
    try {
        FileIO::write_file(output_file, output_data);
    } catch (const std::exception& e) {
        std::cerr << "Error writing output file: " << e.what() << std::endl;
        return 1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Print results
    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout << "║          Compression Complete!          ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";
    std::cout << "Input file:      " << input_file << "\n";
    std::cout << "Output file:     " << output_file << "\n";
    std::cout << "Original size:   " << input_data.size() << " bytes\n";
    std::cout << "Compressed size: " << output_data.size() << " bytes\n";
    std::cout << "Compression ratio: " << (output_data.size() * 100.0 / input_data.size()) << "%\n";
    std::cout << "Space saved:     " << (input_data.size() - output_data.size()) << " bytes ("
              << (100.0 - output_data.size() * 100.0 / input_data.size()) << "%)\n";
    std::cout << "Compression time: " << duration.count() << " ms\n";
    std::cout << "Throughput:      " << (input_data.size() / 1024.0 / (duration.count() / 1000.0))
              << " KB/s\n";

    return 0;
}
