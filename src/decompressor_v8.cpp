/*
 * v8.0 Ultra-Fast LZ77 Decompressor
 *
 * Inverts the token stream produced by compressor_v8.
 * Handles both LZ77_FAST (0x0C) and UNCOMPRESSED (0xFF) formats.
 *
 * Decompression is near-memcpy speed because:
 *   - No hash table: reads input strictly left-to-right, once.
 *   - No entropy decoding: just read a byte, do two memcpys per sequence.
 *   - WildCopy for all non-overlapping copies (8-byte chunks).
 *   - Branch-free fast path when lit_len < 15 and match_len < 19.
 *
 * Token stream format reminder:
 *   [token: 1B] [ext_lit_len: 0+B] [literals: N B] [offset: 2B LE] [ext_match_len: 0+B]
 *   Final sequence has no offset/match (stream ends after literals).
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <cstring>
#include <cstdint>
#include <stdexcept>

#include "utils/file_io.h"
#include "utils/stream_header.h"

using StreamHeader::ModelType;

// ============================================================
// LZ77 decompression engine
// ============================================================
namespace {

static constexpr int MINMATCH = 4;

[[nodiscard]] static inline uint16_t read_u16_le(const uint8_t* p) noexcept {
    uint16_t v; std::memcpy(&v, p, 2); return v;
}

// Copy exactly len bytes in 8-byte chunks, no overshoot.
// Safe for non-overlapping regions and for overlapping regions where src < dst
// and (dst - src) >= 8 (each individual 8-byte memcpy is non-overlapping).
static inline void safe_copy8(uint8_t* dst, const uint8_t* src, size_t len) noexcept {
    uint8_t* dend = dst + len;
    while (dst + 8 <= dend) {
        std::memcpy(dst, src, 8);
        dst += 8; src += 8;
    }
    // Copy remaining 0-7 bytes individually — no overshoot
    while (dst < dend) { *dst++ = *src++; }
}

// ----------------------------------------------------------------
// Core decompression
//
// src      – compressed payload (after the 13-byte file header)
// src_size – byte count of the payload
// dst      – output buffer (pre-allocated to original_size + 8 for slack)
// dst_size – expected decompressed size
//
// Returns actual bytes written; throws on corrupt data.
// ----------------------------------------------------------------
[[nodiscard]] static size_t lz77_decompress(const uint8_t* src, size_t src_size,
                                             uint8_t* dst,  size_t dst_size) {
    const uint8_t* ip    = src;
    const uint8_t* iend  = src + src_size;
    uint8_t*       op    = dst;
    uint8_t*       oend  = dst + dst_size;

    while (true) {
        // ---- Read token ------------------------------------------------
        if (__builtin_expect(ip >= iend, 0))
            throw std::runtime_error("LZ77 decompress: unexpected end of input");

        uint8_t token    = *ip++;
        size_t  lit_len  = token >> 4;

        // ---- Decode extended literal length ----------------------------
        if (__builtin_expect(lit_len == 15, 0)) {
            uint8_t s;
            do {
                if (__builtin_expect(ip >= iend, 0))
                    throw std::runtime_error("LZ77 decompress: truncated literal length");
                s = *ip++;
                lit_len += s;
            } while (s == 255);
        }

        // ---- Copy literals ---------------------------------------------
        if (__builtin_expect(op + lit_len > oend, 0))
            throw std::runtime_error("LZ77 decompress: literal overflow");
        if (__builtin_expect(ip + lit_len > iend, 0))
            throw std::runtime_error("LZ77 decompress: literal out of bounds");

        // Exact-length copy: ip (compressed stream) and op (output) don't overlap,
        // so plain memcpy is safe and doesn't pollute future back-reference sources.
        std::memcpy(op, ip, lit_len);
        op += lit_len;
        ip += lit_len;

        // ---- End-of-stream check (last sequence has no match) ----------
        if (__builtin_expect(ip >= iend, 0)) break;

        // ---- Read match offset (2 bytes LE) ----------------------------
        if (__builtin_expect(ip + 2 > iend, 0))
            throw std::runtime_error("LZ77 decompress: truncated offset");

        uint16_t offset = read_u16_le(ip);
        ip += 2;

        if (__builtin_expect(offset == 0, 0))
            throw std::runtime_error("LZ77 decompress: zero offset (corrupt stream)");

        uint8_t* match = op - offset;
        if (__builtin_expect(match < dst, 0))
            throw std::runtime_error("LZ77 decompress: offset beyond window");

        // ---- Decode match length ---------------------------------------
        size_t match_len = static_cast<size_t>(token & 0x0F) + MINMATCH;

        if (__builtin_expect((token & 0x0F) == 15, 0)) {
            uint8_t s;
            do {
                if (__builtin_expect(ip >= iend, 0))
                    throw std::runtime_error("LZ77 decompress: truncated match length");
                s = *ip++;
                match_len += s;
            } while (s == 255);
        }

        if (__builtin_expect(op + match_len > oend, 0))
            throw std::runtime_error("LZ77 decompress: match overflow");

        // ---- Copy match ------------------------------------------------
        // offset >= 8: each 8-byte memcpy is internally non-overlapping,
        //   so safe_copy8 (8-byte chunks, no overshoot) is correct and fast.
        // offset < 8: must propagate repeated pattern byte-by-byte.
        if (__builtin_expect(static_cast<size_t>(offset) >= 8, 1)) {
            safe_copy8(op, match, match_len);
        } else {
            for (size_t i = 0; i < match_len; i++)
                op[i] = match[i];
        }
        op += match_len;
    }

    return static_cast<size_t>(op - dst);
}

} // anonymous namespace

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "G07 v8.0 - Ultra-Fast LZ77 Lossless Decompressor\n\n";
        std::cout << "Usage: " << argv[0] << " <compressed_file> <output_file>\n";
        return 1;
    }

    try {
        auto t0 = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> input = FileIO::read_file(argv[1]);
        std::cout << "Input: " << argv[1] << " (" << input.size() << " bytes)" << std::endl;

        if (input.empty())
            throw std::runtime_error("Input file is empty");

        auto model_byte = static_cast<ModelType>(input[0]);

        // ---- UNCOMPRESSED format ---------------------------------------
        if (model_byte == ModelType::UNCOMPRESSED) {
            if (input.size() < 9)
                throw std::runtime_error("Corrupt uncompressed header");

            uint64_t orig_size = 0;
            for (int i = 0; i < 8; i++)
                orig_size |= static_cast<uint64_t>(input[1 + i]) << (i * 8);

            if (input.size() != 9 + orig_size)
                throw std::runtime_error("Uncompressed: size mismatch");

            std::vector<uint8_t> out(input.begin() + 9, input.end());
            FileIO::write_file(argv[2], out);

            auto t1 = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            std::cout << "Decompressed: " << out.size() << " bytes (uncompressed) in "
                      << ms << " ms" << std::endl;
            return 0;
        }

        // ---- LZ77_FAST format (0x0C) -----------------------------------
        if (model_byte != ModelType::LZ77_FAST)
            throw std::runtime_error("Not a v8 LZ77 compressed file (unexpected model byte)");

        if (input.size() < 13)
            throw std::runtime_error("Corrupt v8 header: too small");

        // Parse header
        uint64_t orig_size = 0;
        for (int i = 0; i < 8; i++)
            orig_size |= static_cast<uint64_t>(input[1 + i]) << (i * 8);

        uint32_t payload_size = 0;
        for (int i = 0; i < 4; i++)
            payload_size |= static_cast<uint32_t>(input[9 + i]) << (i * 8);

        if (13 + payload_size > input.size())
            throw std::runtime_error("Corrupt v8 header: payload_size exceeds file");

        std::cout << "Original size:  " << orig_size << " bytes" << std::endl;
        std::cout << "Payload size:   " << payload_size << " bytes" << std::endl;

        // Handle empty original file
        if (orig_size == 0) {
            std::vector<uint8_t> empty;
            FileIO::write_file(argv[2], empty);
            std::cout << "Empty file decompressed." << std::endl;
            return 0;
        }

        // Allocate output with 8-byte slack for wild_copy8 overshoot safety
        std::vector<uint8_t> out(orig_size + 8, 0);

        size_t written = lz77_decompress(input.data() + 13, payload_size,
                                          out.data(), orig_size);

        if (written != orig_size)
            throw std::runtime_error("LZ77 decompress: size mismatch ("
                + std::to_string(written) + " vs " + std::to_string(orig_size) + ")");

        out.resize(orig_size);
        FileIO::write_file(argv[2], out);

        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        double speed = (ms > 0) ? (orig_size / 1048576.0) / (ms / 1000.0) : 0.0;

        std::cout << "\n=== Decompression Statistics ===" << std::endl;
        std::cout << "Model:              v8 LZ77 Ultra-Fast" << std::endl;
        std::cout << "Compressed size:    " << input.size() << " bytes" << std::endl;
        std::cout << "Decompressed size:  " << orig_size << " bytes" << std::endl;
        std::cout << "Decompression time: " << ms << " ms";
        if (speed > 0) std::cout << " (" << speed << " MB/s)";
        std::cout << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
