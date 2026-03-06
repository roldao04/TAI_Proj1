#include <iostream>
#include <chrono>
#include <string>
#include <cstring>
#include "arithmetic/range_coder.h"
#include "model/frequency_model.h"
#include "model/context_model.h"
#include "utils/file_io.h"
#include "utils/entropy_calculator.h"
#include "transform/bwt.h"

/*
 * Lossless Data Compressor
 *
 * Compressed file format:
 * - Model type (1 byte): 0=Order-0, 1=Order-1, 3/4=BWT variants, 255=Uncompressed
 * - Original file size (8 bytes, uint64_t)
 * - BWT header (if BWT enabled): block count + primary indices
 * - Model data (Order-0 only: frequency table)
 * - Range-coded data
 */

enum class ModelType {
    ORDER_0 = 0,
    ORDER_1 = 1,
    ORDER_2 = 2,
    ORDER_0_BWT = 3,
    ORDER_1_BWT = 4,
    UNCOMPRESSED = 255
};

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <input_file> <output_file> [OPTIONS]" << std::endl;
    std::cerr << "\nModel Options:" << std::endl;
    std::cerr << "  --model auto      Auto-select best model based on file analysis (default)" << std::endl;
    std::cerr << "  --model order0    Force Order-0 frequency model (fast, universal)" << std::endl;
    std::cerr << "  --model order1    Force Order-1 adaptive model (best compression for low-entropy data)" << std::endl;
    std::cerr << "\nBWT Preprocessing:" << std::endl;
    std::cerr << "  --bwt             Force BWT preprocessing (improves compression on structured data)" << std::endl;
    std::cerr << "  --no-bwt          Disable BWT preprocessing (default: auto-decided)" << std::endl;
    std::cerr << "\nOther Options:" << std::endl;
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

    ModelType model_type = ModelType::ORDER_0;
    bool auto_select = true;
    bool force_mode = false;
    int bwt_preference = 0;

    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--yes" || arg == "-y") {
            force_mode = true;
        } else if (arg == "--bwt") {
            bwt_preference = 1;
        } else if (arg == "--no-bwt") {
            bwt_preference = -1;
        } else if (arg == "--model") {
            if (i + 1 < argc) {
                std::string model_name = argv[i + 1];
                if (model_name == "order0") {
                    model_type = ModelType::ORDER_0;
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
            } else if (entropy > 6.8) {
                model_type = ModelType::ORDER_0;
                std::cout << "Decision: Order-0 (high entropy " << entropy << " > 6.8, marginal compression benefit)" << std::endl;
            } else if (input_data.size() < 102400) {
                model_type = ModelType::ORDER_0;
                std::cout << "Decision: Order-0 (small file < 100KB)" << std::endl;
            } else {
                model_type = ModelType::ORDER_1;
                std::cout << "Decision: Order-1 (entropy " << entropy << " < 6.8, good compression expected)" << std::endl;
            }
        } else {
            std::cout << "Using user-specified model" << std::endl;
        }

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

        bool use_bwt = false;
        std::vector<uint32_t> bwt_primary_indices;

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

            if (use_bwt) {
                if (model_type == ModelType::ORDER_0) {
                    model_type = ModelType::ORDER_0_BWT;
                } else if (model_type == ModelType::ORDER_1) {
                    model_type = ModelType::ORDER_1_BWT;
                }
            }
        }

        std::vector<uint8_t> output_data;

        output_data.push_back(static_cast<uint8_t>(model_type));

        uint64_t original_size = input_data.size();
        for (int i = 0; i < 8; i++) {
            output_data.push_back((original_size >> (i * 8)) & 0xFF);
        }

        std::vector<uint8_t> data_to_encode = input_data;

        if (use_bwt) {
            std::cout << "Applying BWT preprocessing..." << std::endl;
            auto start_bwt = std::chrono::high_resolution_clock::now();

            auto [bwt_output, primary_indices] = BWT::transform_blocks(input_data, 900*1024);
            data_to_encode = bwt_output;
            bwt_primary_indices = primary_indices;

            auto end_bwt = std::chrono::high_resolution_clock::now();
            auto bwt_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_bwt - start_bwt).count();
            std::cout << "BWT preprocessing complete (" << bwt_time << " ms)" << std::endl;
            std::cout << "Number of BWT blocks: " << bwt_primary_indices.size() << std::endl;

            uint32_t block_count = bwt_primary_indices.size();
            for (int i = 0; i < 4; i++) {
                output_data.push_back((block_count >> (i * 8)) & 0xFF);
            }
            for (uint32_t idx : bwt_primary_indices) {
                for (int i = 0; i < 4; i++) {
                    output_data.push_back((idx >> (i * 8)) & 0xFF);
                }
            }
        }

        if (model_type == ModelType::ORDER_0 || model_type == ModelType::ORDER_0_BWT) {
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

        } else if (model_type == ModelType::ORDER_1 || model_type == ModelType::ORDER_2 || model_type == ModelType::ORDER_1_BWT) {
            std::string model_name = (model_type == ModelType::ORDER_1 || model_type == ModelType::ORDER_1_BWT) ? "Order-1" : "Order-2";
            std::cout << "Using " << model_name << " adaptive context model..." << std::endl;
            std::cout << "Encoding with simplified adaptive model..." << std::endl;

            ContextModel model;
            model.set_encoding_method_simple();
            model.init_adaptive();

            RangeEncoder encoder;

            for (size_t idx = 0; idx < data_to_encode.size(); idx++) {
                uint8_t byte = data_to_encode[idx];

                auto steps = model.encode_symbol(byte);
                for (const auto& step : steps) {
                    encoder.encode_symbol(step.cum_freq_low, step.cum_freq_high, step.total_freq);
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
