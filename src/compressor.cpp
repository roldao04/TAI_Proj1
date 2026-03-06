#include <iostream>
#include <chrono>
#include <string>
#include <cstring>
#include "arithmetic/range_coder.h"
#include "model/frequency_model.h"
#include "model/context_model.h"
#include "utils/file_io.h"
#include "utils/entropy_calculator.h"

/*
 * Lossless Data Compressor
 *
 * Supports multiple compression models:
 * - Order-0: Simple frequency model (default, backward compatible)
 * - Order-2: Context-based model using previous 2 bytes
 *
 * Usage:
 *   ./compress <input_file> <output_file> [--model order0|order2]
 *
 * Compressed file format:
 * - Model type (1 byte): 0 = Order-0, 2 = Order-2
 * - Original file size (8 bytes, uint64_t)
 * - Model data (varies by model type)
 * - Arithmetic coded data
 */

enum class ModelType {
    ORDER_0 = 0,
    ORDER_1 = 1,
    ORDER_2 = 2,
    UNCOMPRESSED = 255  // Store uncompressed for incompressible data
};

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <input_file> <output_file> [--model order0|order1|auto] [--yes]" << std::endl;
    std::cerr << "\nOptions:" << std::endl;
    std::cerr << "  --model auto      Auto-select best model based on file analysis (default)" << std::endl;
    std::cerr << "  --model order0    Force Order-0 frequency model (fast, universal)" << std::endl;
    std::cerr << "  --model order1    Force Order-1 adaptive model (best compression for low-entropy data)" << std::endl;
    std::cerr << "  --yes, -y         Skip interactive prompts (useful for automation/benchmarks)" << std::endl;
    std::cerr << "\nAuto-selection rules (based on benchmark results):" << std::endl;
    std::cerr << "  Entropy > 7.5 → UNCOMPRESSED (incompressible, e.g., already compressed)" << std::endl;
    std::cerr << "  Entropy 6.8-7.5 → Order-0 (high entropy, Order-1 too slow for marginal gain)" << std::endl;
    std::cerr << "  File < 100KB → Order-0 (adaptive overhead not worth it)" << std::endl;
    std::cerr << "  Otherwise → Order-1 (low entropy, achieves 20-50% better compression)" << std::endl;
    std::cerr << "\nNotes:" << std::endl;
    std::cerr << "  - Order-1 uses simplified encoding (no PPM Method C exclusions)" << std::endl;
    std::cerr << "  - Order-2 removed (provided no benefit over Order-1)" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string input_filename = argv[1];
    std::string output_filename = argv[2];

    // Default: auto-select based on file size (will be determined after reading file)
    ModelType model_type = ModelType::ORDER_0;
    bool auto_select = true;  // Automatic model selection by default
    bool force_mode = false;   // Skip interactive prompts (for automation/benchmarks)

    // Parse optional arguments
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--yes" || arg == "-y") {
            force_mode = true;
        } else if (arg == "--model") {
            if (i + 1 < argc) {
                std::string model_name = argv[i + 1];
                if (model_name == "order0") {
                    model_type = ModelType::ORDER_0;
                    auto_select = false;  // User override
                } else if (model_name == "order1") {
                    model_type = ModelType::ORDER_1;
                    auto_select = false;  // User override
                } else if (model_name == "order2") {
                    std::cerr << "Error: Order-2 has been removed (provided no benefit over Order-1)" << std::endl;
                    std::cerr << "       Benchmark results showed identical compression ratios with worse performance" << std::endl;
                    std::cerr << "       Use --model order1 instead, or --model auto for automatic selection" << std::endl;
                    return 1;
                } else if (model_name == "auto") {
                    auto_select = true;  // Explicitly request auto-selection
                } else {
                    std::cerr << "Error: Unknown model type '" << model_name << "'" << std::endl;
                    print_usage(argv[0]);
                    return 1;
                }
                i++;  // Skip next argument (model name)
            } else {
                std::cerr << "Error: --model requires an argument (order0, order1, or auto)" << std::endl;
                print_usage(argv[0]);
                return 1;
            }
        }
    }

    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        // Read input file
        std::cout << "Reading input file: " << input_filename << std::endl;
        std::vector<uint8_t> input_data = FileIO::read_file(input_filename);
        std::cout << "Input size: " << input_data.size() << " bytes" << std::endl;

        // Smart model selection based on entropy and file size
        if (auto_select) {
            // Calculate entropy to detect compressibility
            double entropy = EntropyCalculator::calculate(input_data, 8192);
            std::cout << "Detected entropy: " << entropy << " bits/symbol" << std::endl;

            // Decision logic (Improved based on benchmark results)
            if (entropy > 7.5) {
                // Incompressible data (File D: entropy ~8.0, expands to 102%)
                // Better to store uncompressed
                model_type = ModelType::UNCOMPRESSED;
                std::cout << "Decision: UNCOMPRESSED (entropy " << entropy << " > 7.5, incompressible)" << std::endl;

            } else if (entropy > 6.8) {
                // High entropy files (Files E, F: entropy ~7.0-7.3, ratio ~87-88%)
                // Order-1 is very slow (14-18s) and doesn't compress much better
                // Order-0 achieves similar ratio in milliseconds
                model_type = ModelType::ORDER_0;
                std::cout << "Decision: Order-0 (high entropy " << entropy << " > 6.8, marginal compression benefit)" << std::endl;

            } else if (input_data.size() < 102400) {  // < 100KB
                // Small files: Order-0 is faster and header overhead matters
                model_type = ModelType::ORDER_0;
                std::cout << "Decision: Order-0 (small file < 100KB)" << std::endl;

            } else {
                // Low entropy, large files: Order-1 shines! (Files A, B, C, G, H)
                // Achieves 20-50% better compression than Order-0
                model_type = ModelType::ORDER_1;
                std::cout << "Decision: Order-1 (entropy " << entropy << " < 6.8, good compression expected)" << std::endl;
            }
        } else {
            std::cout << "Using user-specified model" << std::endl;
        }

        // Safety check: warn if Order-1 manually selected for high-entropy data
        if (!auto_select && model_type == ModelType::ORDER_1) {
            double entropy = EntropyCalculator::calculate(input_data, 8192);
            if (entropy > 6.8) {
                std::cerr << "\n⚠️  WARNING: Order-1 not recommended for high-entropy files!" << std::endl;
                std::cerr << "    Detected entropy: " << entropy << " bits/symbol (threshold: 6.8)" << std::endl;
                std::cerr << "    Expected issues: Very slow compression (10-30s), possible timeout on decompression" << std::endl;
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

        std::vector<uint8_t> output_data;

        // Write model type marker (1 byte)
        output_data.push_back(static_cast<uint8_t>(model_type));

        // Write original file size (8 bytes)
        uint64_t original_size = input_data.size();
        for (int i = 0; i < 8; i++) {
            output_data.push_back((original_size >> (i * 8)) & 0xFF);
        }

        if (model_type == ModelType::ORDER_0) {
            std::cout << "Using Order-0 frequency model..." << std::endl;

            // Build frequency model
            FrequencyModel model;
            model.build_from_data(input_data);

            // Encode data
            std::cout << "Encoding..." << std::endl;
            RangeEncoder encoder;

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

        } else if (model_type == ModelType::ORDER_1 || model_type == ModelType::ORDER_2) {
            std::string model_name = (model_type == ModelType::ORDER_1) ? "Order-1" : "Order-2";
            std::cout << "Using " << model_name << " adaptive context model..." << std::endl;

            // Initialize adaptive context model with simplified encoding
            ContextModel model;
            model.set_encoding_method_simple();  // Use simplified mode (Phase 1: no exclusions)
            model.init_adaptive();

            // Encode data
            std::cout << "Encoding with simplified adaptive model..." << std::endl;
            RangeEncoder encoder;

            for (size_t idx = 0; idx < input_data.size(); idx++) {
                uint8_t byte = input_data[idx];

                // Get encoding steps (may include escapes)
                auto steps = model.encode_symbol(byte);

                // Encode all steps
                for (const auto& step : steps) {
                    encoder.encode_symbol(step.cum_freq_low, step.cum_freq_high, step.total_freq);
                }

                // Update model adaptively (CRITICAL: both encoder and decoder must do this)
                model.update_frequencies(byte);
                model.update_history(byte);
            }

            // No EOF symbol needed - we rely on file size

            encoder.finish();
            const std::vector<uint8_t>& encoded_data = encoder.get_output();

            // For adaptive PPM, NO model data is stored!
            // Decoder will build the same model by updating frequencies identically
            output_data.insert(output_data.end(), encoded_data.begin(), encoded_data.end());
        } else if (model_type == ModelType::UNCOMPRESSED) {
            std::cout << "Storing UNCOMPRESSED (detected incompressible data)" << std::endl;
            // Just copy the input data as-is (header already written)
            output_data.insert(output_data.end(), input_data.begin(), input_data.end());
        }

        // Write output file
        std::cout << "Writing compressed file: " << output_filename << std::endl;
        FileIO::write_file(output_filename, output_data);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // Statistics
        std::cout << "\n=== Compression Statistics ===" << std::endl;
        std::string model_name;
        if (model_type == ModelType::ORDER_0) model_name = "Order-0";
        else if (model_type == ModelType::ORDER_1) model_name = "Order-1";
        else if (model_type == ModelType::ORDER_2) model_name = "Order-2";
        else if (model_type == ModelType::UNCOMPRESSED) model_name = "Uncompressed";
        std::cout << "Model: " << model_name << std::endl;
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
