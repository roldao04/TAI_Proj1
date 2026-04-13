/**
 * Decompressor v6.0 - Multi-Order PPM Decompressor
 *
 * Decompresses files created by compressor_v6
 * Maintains backward compatibility with v1-v5 formats
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>

// v5.0 components
#include "utils/file_io.h"
#include "utils/stream_header.h"
#include "transform/bwt.h"
#include "transform/mtf.h"
#include "transform/zero_rle.h"
#include "arithmetic/range_coder.h"

// v6.0 components
#include "model/multi_order_ppm.h"
#include "model/context_mixer.h"
#include "model/prediction_utils.h"

struct V6Header {
    uint8_t model_type;          // 0x09 for v6.0
    uint64_t original_size;      // Original file size
    uint64_t processed_size;     // Size after BWT/MTF/ZRLE preprocessing
    uint8_t config_byte;         // BWT/MTF/ZRLE flags
    uint32_t bwt_primary_index;  // BWT primary index (if BWT enabled)
    uint8_t num_orders;          // Number of PPM orders
    std::vector<int> orders;     // Order values
    std::vector<int64_t> mixer_weights;  // Static weights trained on sample (Option 6C)
    bool use_bwt;
    bool use_mtf;
    bool use_zrle;
};

V6Header parse_v6_header(const std::vector<uint8_t>& data, size_t& offset) {
    V6Header header;

    if (data.size() < 12) {
        throw std::runtime_error("File too small to contain valid header");
    }

    // Model type
    header.model_type = data[offset++];

    if (header.model_type != 0x09) {
        throw std::runtime_error("Not a v6.0 compressed file (model type: 0x" +
                                std::to_string((int)header.model_type) + ")");
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

    // Number of orders
    header.num_orders = data[offset++];

    // Order values
    header.orders.reserve(header.num_orders);
    for (int i = 0; i < header.num_orders; i++) {
        header.orders.push_back(data[offset++]);
    }

    // Static mixer weights (N × 8 bytes)
    // These were trained on sample during compression (Option 6C)
    header.mixer_weights.reserve(header.num_orders);
    for (int i = 0; i < header.num_orders; i++) {
        int64_t weight = 0;
        for (int j = 0; j < 8; j++) {
            if (offset >= data.size()) {
                throw std::runtime_error("Unexpected end of file while reading mixer weights");
            }
            weight |= ((int64_t)data[offset++]) << (j * 8);
        }
        header.mixer_weights.push_back(weight);
    }

    return header;
}

std::vector<uint8_t> decode_with_multiorder_ppm(const std::vector<uint8_t>& encoded,
                                                 size_t offset,
                                                 const V6Header& header,
                                                 bool verbose) {
    if (header.processed_size == 0) {
        return std::vector<uint8_t>();
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Initialize multi-order PPM with same configuration
    MultiOrderPPM ppm(header.orders, 8 * 1024 * 1024, 8 * 1024);  // 8M contexts (matches compressor)

    // Initialize context mixer with static weights from file header (Option 6C)
    ContextMixer mixer(header.orders.size(), 0.002);

    // Load static weights (trained on sample during compression)
    if (header.mixer_weights.size() != header.orders.size()) {
        throw std::runtime_error("Mixer weights count mismatch: got " +
                                std::to_string(header.mixer_weights.size()) +
                                ", expected " + std::to_string(header.orders.size()));
    }
    mixer.set_all_weights_fixed(header.mixer_weights);

    if (verbose) {
        std::cout << "Loaded static weights (sample-trained during compression):\n";
        for (size_t i = 0; i < header.mixer_weights.size(); i++) {
            std::cout << "  Order-" << header.orders[i] << ": "
                      << (header.mixer_weights[i] / 65536.0) << "\n";
        }
    }

    // Initialize range decoder
    std::vector<uint8_t> encoded_data(encoded.begin() + offset, encoded.end());
    RangeDecoder decoder(encoded_data);

    std::vector<uint8_t> decoded;
    decoded.reserve(header.processed_size);

    if (verbose) {
        std::cout << "\nDecoding with Multi-Order PPM + Context Mixing...\n";
        std::cout << "Orders: ";
        for (int order : header.orders) std::cout << order << " ";
        std::cout << "\nTarget size: " << header.processed_size << " bytes (after preprocessing)\n";
    }

    // Decode each byte
    size_t progress_interval = header.processed_size / 100;
    if (progress_interval == 0) progress_interval = 1;

    for (size_t i = 0; i < header.processed_size; i++) {
        // Get blended frequency distribution from all PPM orders
        // CRITICAL: This must be IDENTICAL to what encoder used
        // Built from context only, not knowing the byte yet
        uint32_t freqs[256];
        ppm.get_blended_frequencies(freqs, mixer.get_weights_fixed());

        // Build cumulative frequencies for range decoder
        uint32_t cumul[257];
        cumul[0] = 0;
        uint32_t total_freq = 0;
        for (int j = 0; j < 256; j++) {
            cumul[j+1] = cumul[j] + freqs[j];
            total_freq += freqs[j];
        }

        // Decode the actual byte from the bitstream
        uint32_t value = decoder.get_current_count(total_freq);

        // Find which symbol this value corresponds to
        uint8_t decoded_byte = 0;
        for (int j = 0; j < 256; j++) {
            if (value >= cumul[j] && value < cumul[j+1]) {
                decoded_byte = (uint8_t)j;
                break;
            }
        }

        // Update decoder state
        decoder.decode_symbol(cumul[decoded_byte], cumul[decoded_byte+1], total_freq);

        decoded.push_back(decoded_byte);

        // Update models with decoded byte
        ppm.update_all(decoded_byte);

        // DO NOT update mixer weights - they are static (loaded from file header)
        // This ensures deterministic decoding with zero divergence

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
    }

    // CRITICAL VALIDATION: PPM must decode exactly processed_size bytes
    if (decoded.size() != header.processed_size) {
        throw std::runtime_error(
            "PPM decode size mismatch: got " + std::to_string(decoded.size()) +
            " bytes, expected " + std::to_string(header.processed_size) + " bytes"
        );
    }

    return decoded;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <compressed_file> <output_file> [--quiet]\n\n";
        std::cout << "Decompresses files created by compress_v6\n";
        std::cout << "Also supports v1-v5 formats (backward compatibility)\n";
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
        std::cout << "║  Lossless Decompression Tool v6.0       ║\n";
        std::cout << "║  Multi-Order PPM Decompressor           ║\n";
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
    V6Header header;
    try {
        header = parse_v6_header(compressed_data, offset);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing header: " << e.what() << std::endl;
        std::cerr << "\nNote: If this is a v1-v5 compressed file, use the regular decompress tool\n";
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
        std::cout << "  Orders: ";
        for (int order : header.orders) std::cout << order << " ";
        std::cout << "\n";
    }

    // Decode
    std::vector<uint8_t> decoded_data = decode_with_multiorder_ppm(compressed_data, offset, header, verbose);

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

        // CRITICAL VALIDATION: After ZRLE decode, size must equal original
        // (since MTF and BWT don't change data size, only reorder/transform)
        if (decoded_data.size() != header.original_size) {
            throw std::runtime_error(
                "ZRLE decode size mismatch: got " + std::to_string(decoded_data.size()) +
                " bytes, expected " + std::to_string(header.original_size) + " bytes " +
                "(BWT+MTF output size). This means PPM or ZRLE decode corrupted data."
            );
        }
    }

    if (header.use_mtf && header.use_bwt) {
        std::cout << "\nReversing MTF transform...\n";
        size_t before_mtf = decoded_data.size();
        decoded_data = MoveToFront::inverse_transform(decoded_data);
        std::cout << "  MTF reversed: " << before_mtf << " → " << decoded_data.size() << " bytes\n";

        // MTF should not change size
        if (decoded_data.size() != before_mtf) {
            throw std::runtime_error("MTF inverse changed data size - this should never happen!");
        }
    }

    if (header.use_bwt) {
        std::cout << "\nReversing BWT transform...\n";
        std::cout << "  Input size: " << decoded_data.size() << " bytes\n";
        std::cout << "  Primary index: " << header.bwt_primary_index << "\n";
        std::cout << "  Expected size: " << header.original_size << " bytes\n";

        // Debug: check first few bytes
        std::cout << "  First 10 bytes: ";
        for (size_t i = 0; i < std::min((size_t)10, decoded_data.size()); i++) {
            std::cout << (int)decoded_data[i] << " ";
        }
        std::cout << "\n";

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
