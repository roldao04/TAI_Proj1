/**
 * Compressor v6.0 - Maximum Compression Multi-Order PPM
 *
 * Focus: Compression ratio above all else
 * Features:
 * - Multi-order PPM: Orders 1, 2, 4, 6, 8, 12, 16, 24, 32
 * - Context mixing with adaptive weights
 * - BWT/MTF/ZRLE preprocessing (from v5.0)
 * - Byte-level encoding with range coder
 *
 * Trade-offs:
 * - 10-100× slower than v5.0
 * - Uses 2-4 GB RAM
 * - Target: < 50% compression ratio (vs v5.0's 54.73%)
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>

// v5.0 components (reuse)
#include "utils/file_io.h"
#include "utils/entropy_calculator.h"
#include "utils/stream_header.h"
#include "transform/bwt.h"
#include "transform/mtf.h"
#include "transform/zero_rle.h"
#include "arithmetic/range_coder.h"

// v6.0 components (new)
#include "model/multi_order_ppm.h"
#include "model/context_mixer.h"
#include "model/prediction_utils.h"

// Configuration
struct V6Config {
    std::vector<int> orders = {1, 2, 3, 4, 5}; // Start with 5 orders (conservative)
    size_t hash_table_size = 8 * 1024 * 1024;  // 8M contexts (~768MB RAM)
    size_t max_history = 8 * 1024;             // 8KB history
    double learning_rate = 0.002;              // Mixer learning rate
    bool use_bwt = true;                       // Enable BWT preprocessing
    bool use_mtf = true;                       // Enable MTF transform
    bool use_zrle = true;                      // Enable zero-run RLE
    bool verbose = true;                       // Print statistics
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <input_file> <output_file> [options]\n\n";
    std::cout << "v6.0 Options (Maximum Compression):\n";
    std::cout << "  --orders <list>       Comma-separated orders (default: 1,2,4,6,8,12,16,24,32)\n";
    std::cout << "  --hash-size <M>       Hash table size in millions (default: 16)\n";
    std::cout << "  --learning-rate <lr>  Mixer learning rate (default: 0.002)\n";
    std::cout << "  --no-bwt              Disable BWT preprocessing\n";
    std::cout << "  --no-mtf              Disable MTF transform\n";
    std::cout << "  --no-zrle             Disable zero-run RLE\n";
    std::cout << "  --quiet               Minimal output\n";
    std::cout << "  --yes, -y             Skip prompts\n";
    std::cout << "\nWarning: v6.0 is 10-100× slower than v5.0 but achieves better compression\n";
}

/**
 * Encode data using multi-order PPM with context mixing
 * Uses static weights (pre-trained on sample) for deterministic compression
 */
std::vector<uint8_t> encode_with_multiorder_ppm(const std::vector<uint8_t>& data,
                                                 const V6Config& config,
                                                 const std::vector<int64_t>& static_weights) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Initialize multi-order PPM
    MultiOrderPPM ppm(config.orders, config.hash_table_size, config.max_history);

    // Initialize context mixer with static weights (pre-trained on sample)
    ContextMixer mixer(config.orders.size(), config.learning_rate);
    mixer.set_all_weights_fixed(static_weights);

    // Initialize range coder
    std::vector<uint8_t> encoded;
    RangeEncoder encoder;

    // Pre-allocate output buffer to prevent repeated reallocations
    // Estimate: input_size * 10.0 for worst case (handles expansion + header overhead)
    // Multi-order PPM can temporarily expand data significantly before converging
    // especially on small/medium files where models haven't adapted yet
    // NOTE: Early in compression, with uniform frequency distributions, range coder
    // can expand data by 8-10x per byte until model adapts
    size_t reserve_size = data.size() * 10 + 65536;  // 10x + 64KB safety margin
    encoder.reserve_output(reserve_size);

    if (config.verbose) {
        std::cout << "Reserved " << (reserve_size / 1024) << " KB for range encoder output\n";
    }

    if (config.verbose) {
        std::cout << "\nEncoding with Multi-Order PPM + Context Mixing...\n";
        std::cout << "Orders: ";
        for (int order : config.orders) std::cout << order << " ";
        std::cout << "\nData size: " << data.size() << " bytes\n";
    }

    // Process each byte
    size_t progress_interval = data.size() / 100;
    if (progress_interval == 0) progress_interval = 1;

    for (size_t i = 0; i < data.size(); i++) {
        uint8_t byte = data[i];

        try {
            // Get blended frequency distribution from all PPM orders
            // This distribution is built BEFORE knowing the byte value,
            // so encoder and decoder will build identical distributions
            uint32_t freqs[256];
            try {
                ppm.get_blended_frequencies(freqs, mixer.get_weights_fixed());
            } catch (const std::bad_alloc& e) {
                std::cerr << "\nError at byte " << i << " in get_blended_frequencies: " << e.what() << std::endl;
                std::cerr << "Hash table size: " << ppm.get_hash_table_usage() << " entries" << std::endl;
                std::cerr << "Memory usage: " << (ppm.get_memory_usage() / (1024*1024)) << " MB" << std::endl;
                throw;
            }

            // Build cumulative frequencies for range coder
            uint32_t cumul[257];
            cumul[0] = 0;
            uint32_t total_freq = 0;
            for (int j = 0; j < 256; j++) {
                cumul[j+1] = cumul[j] + freqs[j];
                total_freq += freqs[j];
            }

            // SAFETY CHECK: Validate total_freq is reasonable
            // Range coder requires: 0 < total_freq < (range / 256) to work correctly
            // Typically total_freq should be ~10000 (OUTPUT_SCALE)
            if (total_freq == 0) {
                std::cerr << "\nFATAL ERROR at byte " << i << ": total_freq is 0!\n";
                std::cerr << "This means all frequencies are 0, which should never happen.\n";
                throw std::runtime_error("Invalid frequency distribution: total_freq = 0");
            }
            if (total_freq > 1000000) {  // 100x larger than expected
                std::cerr << "\nFATAL ERROR at byte " << i << ":\n";
                std::cerr << "  total_freq = " << total_freq << " (expected ~10000)\n";
                std::cerr << "  This is " << (total_freq / 10000) << "x too large!\n";
                std::cerr << "  cumul[" << (int)byte << "] = " << cumul[byte] << "\n";
                std::cerr << "  cumul[" << ((int)byte+1) << "] = " << cumul[byte+1] << "\n";
                std::cerr << "  freq[" << (int)byte << "] = " << freqs[byte] << "\n";
                std::cerr << "  First 10 freqs: [";
                for (int k = 0; k < 10; k++) {
                    std::cerr << freqs[k];
                    if (k < 9) std::cerr << ",";
                }
                std::cerr << "]\n";
                throw std::runtime_error("Invalid frequency distribution: total_freq too large");
            }

            // Encode the actual byte using this distribution
            encoder.encode_symbol(cumul[byte], cumul[byte+1], total_freq);

            // Update models with actual byte
            // Get predictions for mixer weight updates
            std::vector<Prediction> predictions;
            try {
                predictions = ppm.predict_all(byte);
            } catch (const std::bad_alloc& e) {
                std::cerr << "\nError at byte " << i << " in predict_all: " << e.what() << std::endl;
                throw;
            }

            try {
                ppm.update_all(byte);
            } catch (const std::bad_alloc& e) {
                std::cerr << "\nError at byte " << i << " in update_all: " << e.what() << std::endl;
                std::cerr << "Hash table size: " << ppm.get_hash_table_usage() << " entries" << std::endl;
                throw;
            }

            // DO NOT update mixer weights - they are static (pre-trained on sample)
            // This ensures deterministic encoding/decoding with zero divergence

            // Progress indicator
            if (config.verbose && i % progress_interval == 0) {
                int percent = (int)((i * 100) / data.size());
                std::cout << "\rProgress: " << percent << "%" << std::flush;
            }
        } catch (const std::exception& e) {
            std::cerr << "\nError at byte " << i << ": " << e.what() << std::endl;
            throw;
        }
    }

    // Finish encoding
    encoder.finish();
    encoded = encoder.get_output();

    if (config.verbose) {
        std::cout << "\rProgress: 100%\n";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "Encoding time: " << duration.count() << " ms\n";
        std::cout << "Encoded size: " << encoded.size() << " bytes\n";
        std::cout << "Compression ratio: " << (encoded.size() * 100.0 / data.size()) << "%\n";

        // Print mixer statistics (static weights)
        std::cout << "\nStatic mixer weights (trained on sample):\n";
        const auto& weights = mixer.get_weights();
        for (size_t i = 0; i < weights.size(); i++) {
            std::cout << "  Order-" << config.orders[i] << ": " << weights[i] << "\n";
        }

        // Print PPM statistics
        std::cout << "\nPPM hash table usage: "
                  << ppm.get_hash_table_usage() << " / "
                  << config.hash_table_size << " contexts ("
                  << (ppm.get_hash_table_usage() * 100.0 / config.hash_table_size) << "%)\n";
    }

    // Return encoded data (weights will be trained symmetrically on decoder side)
    return encoded;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];

    // Parse configuration
    V6Config config;
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
    std::cout << "║  Lossless Compression Tool v6.0         ║\n";
    std::cout << "║  Maximum Compression Multi-Order PPM    ║\n";
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

    // BWT auto-selection DISABLED for v6 optimization testing
    // Pure PPM works better without BWT's byte reordering
    std::cout << "BWT preprocessing: " << (config.use_bwt ? "ENABLED" : "DISABLED") << " (manual config)\n";
    std::cout << "MTF transform: " << (config.use_mtf ? "ENABLED" : "DISABLED") << " (manual config)\n";
    std::cout << "ZRLE: " << (config.use_zrle ? "ENABLED" : "DISABLED") << " (manual config)\n";

    // Warning about speed
    if (!skip_prompts) {
        std::cout << "\n⚠️  Warning: v6.0 is significantly slower than v5.0 (10-100× compression time)\n";
        std::cout << "   Estimated time for " << (input_data.size() / 1024) << " KB: "
                  << ((input_data.size() / 1024) * 0.1) << " - "
                  << ((input_data.size() / 1024) * 1.0) << " seconds\n";
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

    // Preprocessing pipeline
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

    // Auto-selection for ZRLE: only apply if it reduces size AND no rank-255 bug (from v5.0)
    if (mtf_applied) {
        // Check for rank-255 bug: ZRLE fails when MTF output contains byte 255
        bool has_rank255 = std::any_of(processed_data.begin(), processed_data.end(),
                                       [](uint8_t b){ return b == 255; });

        if (has_rank255) {
            std::cout << "\n[3/4] Zero-run RLE: SKIPPED (MTF output contains rank-255, known ZRLE bug)\n";
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
                std::cout << "\n[3/4] Zero-run RLE: SKIPPED (no size reduction: "
                          << processed_data.size() << " -> " << zrle_candidate.size() << " bytes)\n";
                config.use_zrle = false;
            }
        }
    }

    // ========== SAMPLE-BASED WEIGHT TRAINING ==========
    std::cout << "\n[4a/4] Training Phase: Analyzing file to determine optimal weights...\n";

    size_t training_sample_size = std::min(processed_data.size(), (size_t)102400); // 100KB max
    std::cout << "Training sample: " << training_sample_size << " bytes ("
              << (training_sample_size * 100.0 / processed_data.size()) << "% of file)\n";

    // Initialize training PPM and mixer
    MultiOrderPPM ppm_trainer(config.orders, config.hash_table_size, config.max_history);
    ContextMixer mixer_trainer(config.orders.size(), config.learning_rate);

    // Train on sample
    for (size_t i = 0; i < training_sample_size; i++) {
        uint8_t byte = processed_data[i];
        std::vector<Prediction> predictions = ppm_trainer.predict_all(byte);
        mixer_trainer.update(predictions, (byte & 0x80) != 0);
        ppm_trainer.update_all(byte);
    }

    // Extract static weights from training
    std::vector<int64_t> static_weights = mixer_trainer.get_weights_fixed();

    std::cout << "Trained static weights:\n";
    for (size_t i = 0; i < static_weights.size(); i++) {
        std::cout << "  Order-" << config.orders[i] << ": "
                  << (static_weights[i] / 65536.0) << "\n";
    }
    std::cout << "These weights will be used (frozen) for entire file.\n";

    // Encode with multi-order PPM using static weights
    std::cout << "\n[4b/4] Encoding with Multi-Order PPM (using static weights)...\n";
    std::vector<uint8_t> encoded_data = encode_with_multiorder_ppm(processed_data, config, static_weights);

    // Write output file with header
    std::vector<uint8_t> output_data;

    // Header: model type (0x09 = CONTEXT_MIXING_V6)
    output_data.push_back(0x09);

    // Original size (8 bytes, little-endian)
    uint64_t original_size = input_data.size();
    for (int i = 0; i < 8; i++) {
        output_data.push_back((uint8_t)(original_size >> (i * 8)));
    }

    // Processed size (8 bytes, little-endian) - size after BWT/MTF/ZRLE
    uint64_t processed_size = processed_data.size();
    for (int i = 0; i < 8; i++) {
        output_data.push_back((uint8_t)(processed_size >> (i * 8)));
    }

    // Config byte (store what actually ran, not what was requested)
    uint8_t config_byte = 0;
    if (bwt_applied) config_byte |= 0x01;
    if (mtf_applied) config_byte |= 0x02;
    if (config.use_zrle && mtf_applied) config_byte |= 0x04;  // ZRLE only if MTF ran
    output_data.push_back(config_byte);

    // BWT primary index (4 bytes, little-endian) - only if BWT applied
    if (bwt_applied) {
        for (int i = 0; i < 4; i++) {
            output_data.push_back((uint8_t)(bwt_primary_index >> (i * 8)));
        }
    }

    // Number of orders
    output_data.push_back((uint8_t)config.orders.size());

    // Order values
    for (int order : config.orders) {
        output_data.push_back((uint8_t)order);
    }

    // Static mixer weights (N × 8 bytes = 32 bytes for 4 orders)
    // These are trained on sample and frozen for entire file (Option 6C)
    for (int64_t weight : static_weights) {
        for (int i = 0; i < 8; i++) {
            output_data.push_back((uint8_t)(weight >> (i * 8)));
        }
    }

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
