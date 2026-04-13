#include "model/prng_model.h"

#include <cstring>

namespace {

/**
 * glibc TYPE_3 rand() simulation.
 *
 * The algorithm uses a degree-31 trinomial: x^31 + x^3 + 1
 * with a 31-word (124-byte) state table.
 *
 * Initialization (srand):
 *   state[0] = seed
 *   state[i] = (16807 * state[i-1]) mod 2147483647   for i in 1..30
 *   Then 310 warm-up calls to advance the state.
 *
 * Generation (rand):
 *   state[fptr] = (state[fptr] + state[rptr]) mod 2^32
 *   result = state[fptr] >> 1   (31-bit positive value)
 *   Advance fptr, rptr cyclically through 31 positions.
 *
 * The file stores rand() & 0xFF (low byte of the 31-bit result).
 */
struct GlibcRand {
    static constexpr int DEG = 31;
    static constexpr int SEP = 3;

    int32_t state[DEG];
    int fptr;
    int rptr;

    void srand(uint32_t seed) {
        // Ensure seed is not zero (glibc clamps seed=0 to seed=1)
        if (seed == 0) seed = 1;

        state[0] = static_cast<int32_t>(seed);
        for (int i = 1; i < DEG; i++) {
            int64_t val = static_cast<int64_t>(16807) * state[i - 1];
            state[i] = static_cast<int32_t>(val % 2147483647);
            if (state[i] < 0) state[i] += 2147483647;
        }

        fptr = SEP;
        rptr = 0;

        // 310 warm-up iterations (glibc does 10 * DEG)
        for (int i = 0; i < 310; i++) {
            rand();
        }
    }

    int32_t rand() {
        int32_t val = state[fptr] + state[rptr];
        state[fptr] = val;
        int32_t result = static_cast<uint32_t>(val) >> 1;

        if (++fptr >= DEG) fptr = 0;
        if (++rptr >= DEG) rptr = 0;

        return result;
    }
};

} // namespace

PrngModel::DetectionResult PrngModel::detect(const std::vector<uint8_t>& data,
                                              uint32_t max_seed,
                                              size_t prefix_len) {
    if (data.empty()) {
        return {false, Algorithm::GLIBC_RAND, 0};
    }

    // Clamp prefix to actual data size
    if (prefix_len > data.size()) {
        prefix_len = data.size();
    }

    // Phase 1: Quick prefix match for each seed
    for (uint32_t seed = 0; seed <= max_seed; seed++) {
        GlibcRand rng;
        rng.srand(seed);

        bool match = true;
        for (size_t i = 0; i < prefix_len; i++) {
            uint8_t generated = static_cast<uint8_t>(rng.rand() & 0xFF);
            if (generated != data[i]) {
                match = false;
                break;
            }
        }

        if (!match) continue;

        // Phase 2: Full verification (prefix already matched)
        // Continue from where we left off
        bool full_match = true;
        for (size_t i = prefix_len; i < data.size(); i++) {
            uint8_t generated = static_cast<uint8_t>(rng.rand() & 0xFF);
            if (generated != data[i]) {
                full_match = false;
                break;
            }
        }

        if (full_match) {
            return {true, Algorithm::GLIBC_RAND, seed};
        }
    }

    return {false, Algorithm::GLIBC_RAND, 0};
}

std::vector<uint8_t> PrngModel::generate(Algorithm algo,
                                          uint32_t seed,
                                          size_t length) {
    switch (algo) {
        case Algorithm::GLIBC_RAND:
            return generate_glibc_rand(seed, length);
    }
    return {};
}

std::vector<uint8_t> PrngModel::generate_glibc_rand(uint32_t seed, size_t length) {
    GlibcRand rng;
    rng.srand(seed);

    std::vector<uint8_t> output(length);
    for (size_t i = 0; i < length; i++) {
        output[i] = static_cast<uint8_t>(rng.rand() & 0xFF);
    }
    return output;
}
