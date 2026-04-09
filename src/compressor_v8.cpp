/*
 * v8.0 Ultra-Fast LZ77 Compressor
 *
 * Algorithm: LZ4-style LZ77 dictionary matching
 *   - 16384-entry hash table (64KB) fits entirely in L1 cache
 *   - Single-entry buckets: collisions overwrite (no chains, O(1) lookup)
 *   - Skip acceleration: step grows when no matches found (near-memcpy speed on incompressible data)
 *   - Byte-aligned token format: zero bit manipulation
 *   - WildCopy (8-byte memcpy loops): compiler emits single mov on x86
 *   - 8-byte XOR + ctzll for match extension (no byte-by-byte comparison)
 *   - No entropy coding pass — token stream IS the output
 *
 * File format:
 *   [0x0C: 1B] [original_size: 8B LE] [compressed_payload_size: 4B LE] [LZ77 token stream]
 *
 * Token stream (one sequence per match):
 *   [token: 1B] [ext_lit_len: 0+B] [literals: N B] [offset: 2B LE] [ext_match_len: 0+B]
 *   Final sequence: [token: 1B] [ext_lit_len: 0+B] [literals: N B]  (no offset/match)
 *
 *   token high nibble = literal_length  (0–14, or 15 = extended)
 *   token low  nibble = match_length-4  (0–14, or 15 = extended)
 *   Extended length: keep appending 255-valued bytes; first byte < 255 ends the run.
 *
 * Uncompressed fallback (entropy > 7.5 or LZ77 expands data):
 *   [0xFF: 1B] [original_size: 8B LE] [raw data]
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <cstring>
#include <cstdint>
#include <stdexcept>

#include "utils/file_io.h"
#include "utils/entropy_calculator.h"
#include "utils/stream_header.h"

using StreamHeader::ModelType;

// ============================================================
// LZ77 engine
// ============================================================
namespace {

// --- Algorithm constants (spec-compatible with LZ4) ---
static constexpr int  MINMATCH      = 4;
static constexpr int  LASTLITERALS  = 5;      // last N bytes always emitted as literals
static constexpr int  MFLIMIT       = 12;     // stop match search this many bytes before end
static constexpr int  HASHLOG       = 14;     // 2^14 = 16384 entries × 4 B = 64 KB
static constexpr int  HASHTABLESIZE = 1 << HASHLOG;
static constexpr int  MAX_OFFSET    = 65535;  // 2-byte back-reference distance
static constexpr int  SKIP_TRIGGER  = 6;      // step doubles every 2^6 = 64 consecutive misses

// Unaligned memory helpers — compiler generates a single mov on x86 with -O3.
[[nodiscard]] static inline uint32_t read_u32(const uint8_t* p) noexcept {
    uint32_t v; std::memcpy(&v, p, 4); return v;
}
[[nodiscard]] static inline uint64_t read_u64(const uint8_t* p) noexcept {
    uint64_t v; std::memcpy(&v, p, 8); return v;
}
static inline void write_u16_le(uint8_t* p, uint16_t v) noexcept {
    std::memcpy(p, &v, 2);   // assumes little-endian host (x86/x64/ARM LE)
}

// Knuth multiplicative hash — distributes 4-byte sequences across the hash table.
[[nodiscard]] static inline uint32_t hash4(const uint8_t* p) noexcept {
    return (read_u32(p) * 2654435761U) >> (32 - HASHLOG);
}

// Copy in 8-byte chunks; may overshoot dstEnd by up to 7 bytes.
// Caller must ensure the output buffer has ≥8 bytes of slack.
static inline void wild_copy8(uint8_t* __restrict__ dst,
                               const uint8_t* __restrict__ src,
                               uint8_t* dstEnd) noexcept {
    do {
        std::memcpy(dst, src, 8);
        dst += 8; src += 8;
    } while (dst < dstEnd);
}

// Count matching bytes between ip/ref up to limit.
// Processes 8 bytes at once via XOR; __builtin_ctzll finds first differing bit.
[[nodiscard]] static inline size_t count_match(const uint8_t* ip,
                                                const uint8_t* ref,
                                                const uint8_t* limit) noexcept {
    const uint8_t* start = ip;
    while (ip + 8 <= limit) {
        uint64_t diff = read_u64(ip) ^ read_u64(ref);
        if (diff) {
            ip += static_cast<unsigned>(__builtin_ctzll(diff)) >> 3;
            return static_cast<size_t>(ip - start);
        }
        ip += 8; ref += 8;
    }
    while (ip < limit && *ip == *ref) { ip++; ref++; }
    return static_cast<size_t>(ip - start);
}

// Emit the variable-length "extra bytes" for lengths >= 15.
static inline uint8_t* emit_ext_len(uint8_t* op, unsigned remaining) noexcept {
    for (; remaining >= 255; remaining -= 255) *op++ = 255;
    *op++ = static_cast<uint8_t>(remaining);
    return op;
}

// ----------------------------------------------------------------
// Core LZ77 compression loop
//
// dst_cap must be >= src_size + src_size/255 + 32  (worst-case expansion)
// Returns number of bytes written to dst.
// ----------------------------------------------------------------
[[nodiscard]] static size_t lz77_compress(const uint8_t* __restrict__ src,
                                           size_t src_size,
                                           uint8_t* __restrict__ dst,
                                           size_t /*dst_cap*/) noexcept {
    uint32_t ht[HASHTABLESIZE] = {};   // 64 KB on stack — stays in L1 cache

    const uint8_t* ip         = src;
    const uint8_t* anchor     = src;
    const uint8_t* iend       = src + src_size;
    const uint8_t* mflimit    = iend - MFLIMIT;
    const uint8_t* matchlimit = iend - LASTLITERALS;
    uint8_t* op = dst;

    if (src_size < static_cast<size_t>(MFLIMIT + 1))
        goto last_literals;

    // Seed hash with position 0, start scanning from 1
    ht[hash4(ip)] = 0;
    ip++;

    for (;;) {
        // ----------------------------------------------------------------
        // Phase 1 — find a 4-byte match using skip-accelerated scan
        // ----------------------------------------------------------------
        const uint8_t* match;
        {
            const uint8_t* fwd = ip;
            int step      = 1;
            int search_nb = 1 << SKIP_TRIGGER;   // misses before step increments

            for (;;) {
                uint32_t h = hash4(fwd);
                ip  = fwd;
                fwd += step;
                step = (search_nb++ >> SKIP_TRIGGER);   // step grows ~every 64 misses

                if (__builtin_expect(fwd > mflimit, 0))
                    goto last_literals;

                uint32_t ref_pos = ht[h];
                ht[h] = static_cast<uint32_t>(ip - src);   // always update table

                match = src + ref_pos;
                if (__builtin_expect(
                        static_cast<unsigned>(ip - match) <= static_cast<unsigned>(MAX_OFFSET)
                        && read_u32(ip) == read_u32(match), 0))
                    break;
            }
        }

        // ----------------------------------------------------------------
        // Phase 1.5 — extend match backwards (catch-up before anchor)
        // ----------------------------------------------------------------
        while (ip > anchor && match > src && ip[-1] == match[-1]) {
            ip--; match--;
        }

        // ----------------------------------------------------------------
        // Phase 2 — emit literals
        // ----------------------------------------------------------------
        {
            unsigned lit_len = static_cast<unsigned>(ip - anchor);
            uint8_t* token   = op++;   // reserve token byte; fill low nibble later

            if (__builtin_expect(lit_len >= 15, 0)) {
                *token = 0xF0;
                op = emit_ext_len(op, lit_len - 15);
            } else {
                *token = static_cast<uint8_t>(lit_len << 4);
            }

            // Copy literal bytes (wild_copy8 overshoots are fine — buffer has slack)
            if (lit_len <= 16) {
                std::memcpy(op, anchor, 16);
            } else {
                wild_copy8(op, anchor, op + lit_len);
            }
            op += lit_len;

            // ----------------------------------------------------------------
            // Phase 3+4 — encode offset + match length into token low nibble
            // ----------------------------------------------------------------
    encode_match:
            write_u16_le(op, static_cast<uint16_t>(ip - match));
            op += 2;

            size_t ml = count_match(ip + MINMATCH, match + MINMATCH, matchlimit);
            ip += ml + MINMATCH;

            if (__builtin_expect(ml >= 15, 0)) {
                *token |= 0x0F;
                op = emit_ext_len(op, static_cast<unsigned>(ml) - 15);
            } else {
                *token |= static_cast<uint8_t>(ml);
            }

            anchor = ip;
            if (__builtin_expect(ip > mflimit, 0)) goto done;

            // Hash the position two bytes back (fills gap left by the match)
            ht[hash4(ip - 2)] = static_cast<uint32_t>((ip - 2) - src);

            // Try an immediate re-match at ip (avoids re-entering the search loop)
            {
                uint32_t h       = hash4(ip);
                uint32_t ref_pos = ht[h];
                ht[h] = static_cast<uint32_t>(ip - src);
                match  = src + ref_pos;

                if (static_cast<unsigned>(ip - match) <= static_cast<unsigned>(MAX_OFFSET)
                    && read_u32(ip) == read_u32(match)) {
                    token  = op++;
                    *token = 0;    // 0 literals in this sequence
                    goto encode_match;
                }
            }

            ip++;
        }
        continue;   // back to outer for(;;)
    done:
        break;
    }

last_literals:
    {
        unsigned last_run = static_cast<unsigned>(iend - anchor);
        if (__builtin_expect(last_run >= 15, 0)) {
            *op++ = 0xF0;
            op = emit_ext_len(op, last_run - 15);
        } else {
            *op++ = static_cast<uint8_t>(last_run << 4);
        }
        std::memcpy(op, anchor, last_run);
        op += last_run;
    }

    return static_cast<size_t>(op - dst);
}

} // anonymous namespace

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "G07 v8.0 - Ultra-Fast LZ77 Lossless Compression\n\n";
        std::cout << "Usage: " << argv[0] << " <input_file> <output_file>\n\n";
        std::cout << "Algorithm: LZ4-style LZ77 (byte-aligned tokens, no entropy coding)\n";
        std::cout << "Target:    >500 MB/s compress, >2 GB/s decompress\n";
        return 1;
    }

    try {
        auto t0 = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> input = FileIO::read_file(argv[1]);
        uint64_t orig_size = input.size();
        std::cout << "Input: " << argv[1] << " (" << orig_size << " bytes)" << std::endl;

        // ---- Empty file ------------------------------------------------
        if (orig_size == 0) {
            std::vector<uint8_t> out(13, 0);
            out[0] = static_cast<uint8_t>(ModelType::LZ77_FAST);
            FileIO::write_file(argv[2], out);
            std::cout << "Empty file stored." << std::endl;
            return 0;
        }

        // ---- Entropy check: near-random data compresses poorly ----------
        double entropy = EntropyCalculator::calculate(input, 8192);
        std::cout << "Entropy: " << entropy << " bits/symbol" << std::endl;

        auto store_uncompressed = [&](const char* reason) {
            std::cout << "Decision: UNCOMPRESSED (" << reason << ")" << std::endl;
            std::vector<uint8_t> out;
            out.reserve(9 + orig_size);
            out.push_back(static_cast<uint8_t>(ModelType::UNCOMPRESSED));
            for (int i = 0; i < 8; i++)
                out.push_back(static_cast<uint8_t>((orig_size >> (i * 8)) & 0xFF));
            out.insert(out.end(), input.begin(), input.end());
            FileIO::write_file(argv[2], out);
        };

        if (entropy > 7.5) {
            store_uncompressed("entropy > 7.5");
            auto t1 = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            std::cout << "Time: " << ms << " ms" << std::endl;
            return 0;
        }

        // ---- LZ77 compression ------------------------------------------
        // Worst-case: every byte forces a literal token → src + src/255 + 16
        // Add 32 bytes slack for wild_copy8 overshoot safety
        size_t buf_cap = orig_size + orig_size / 255 + 32;
        std::vector<uint8_t> payload(buf_cap);

        size_t payload_size = lz77_compress(input.data(), orig_size,
                                             payload.data(), buf_cap);
        payload.resize(payload_size);

        // Fall back to uncompressed if LZ77 expanded the data
        if (payload_size + 13 >= orig_size + 9) {
            store_uncompressed("LZ77 expanded data");
            auto t1 = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            std::cout << "Time: " << ms << " ms" << std::endl;
            return 0;
        }

        // Assemble final output: header (13 B) + payload
        std::vector<uint8_t> out;
        out.reserve(13 + payload_size);

        out.push_back(static_cast<uint8_t>(ModelType::LZ77_FAST));
        for (int i = 0; i < 8; i++)
            out.push_back(static_cast<uint8_t>((orig_size >> (i * 8)) & 0xFF));
        uint32_t ps = static_cast<uint32_t>(payload_size);
        for (int i = 0; i < 4; i++)
            out.push_back(static_cast<uint8_t>((ps >> (i * 8)) & 0xFF));
        out.insert(out.end(), payload.begin(), payload.end());

        FileIO::write_file(argv[2], out);

        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        double speed = (ms > 0) ? (orig_size / 1048576.0) / (ms / 1000.0) : 0.0;

        std::cout << "\n=== Compression Statistics ===" << std::endl;
        std::cout << "Model:             v8 LZ77 Ultra-Fast" << std::endl;
        std::cout << "Original size:     " << orig_size << " bytes" << std::endl;
        std::cout << "Compressed size:   " << out.size() << " bytes" << std::endl;
        std::cout << "Compression ratio: " << (100.0 * out.size() / orig_size) << "%" << std::endl;
        std::cout << "Bits per symbol:   " << (8.0 * out.size() / orig_size) << std::endl;
        std::cout << "Compression time:  " << ms << " ms";
        if (speed > 0) std::cout << " (" << speed << " MB/s)";
        std::cout << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
