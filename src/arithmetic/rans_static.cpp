#include "arithmetic/rans_static.h"
#include "arithmetic/rans_byte.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <numeric>

// ============================================================================
// Frequency scaling: spread raw counts into exactly FREQ_SUM slots.
//
// Algorithm (used by many rANS implementations):
//   1. Give each present symbol at least 1 slot.
//   2. Distribute remaining slots proportionally.
//   3. Adjust for rounding errors so sum == FREQ_SUM exactly.
// ============================================================================

std::array<uint16_t, RansStaticCoder::ALPHABET>
RansStaticCoder::build_encode_table(const std::array<uint32_t, RansStaticCoder::ALPHABET>& raw_freq)
{
    // Count present symbols and total raw count
    uint64_t total_raw = 0;
    int present = 0;
    for (int s = 0; s < ALPHABET; s++) {
        if (raw_freq[s] > 0) { total_raw += raw_freq[s]; present++; }
    }

    if (present == 0 || total_raw == 0)
        throw std::runtime_error("rANS: empty frequency table");

    std::array<uint16_t, ALPHABET> scaled{};

    // First pass: proportional allocation (floor), guarantee >= 1 per symbol
    int remaining = FREQ_SUM;
    for (int s = 0; s < ALPHABET; s++) {
        if (raw_freq[s] == 0) { scaled[s] = 0; continue; }
        uint32_t slot = static_cast<uint32_t>(
            (static_cast<uint64_t>(raw_freq[s]) * FREQ_SUM) / total_raw);
        if (slot == 0) slot = 1;
        scaled[s] = static_cast<uint16_t>(slot);
        remaining -= slot;
    }

    // Second pass: fix rounding — add or remove 1 from largest symbols
    if (remaining != 0) {
        if (remaining > 0) {
            // Under-allocated: give extra slots to symbols with largest deficit
            struct Adj { int sym; int64_t delta; };
            std::vector<Adj> adjs;
            adjs.reserve(ALPHABET);
            for (int s = 0; s < ALPHABET; s++) {
                if (raw_freq[s] == 0) continue;
                int64_t ideal = static_cast<int64_t>(raw_freq[s]) * FREQ_SUM;
                int64_t got   = static_cast<int64_t>(scaled[s]) * total_raw;
                adjs.push_back({s, ideal - got});
            }
            std::sort(adjs.begin(), adjs.end(), [](const Adj& a, const Adj& b){
                return a.delta > b.delta;
            });
            for (int i = 0; i < remaining && i < (int)adjs.size(); i++) {
                scaled[adjs[i].sym]++;
            }
        } else {
            // Over-allocated (common after BWT+MTF with many rare symbols bumped to 1):
            // Take from symbols with scaled > 1, largest counts first
            while (remaining < 0) {
                bool progress = false;
                for (int s = 0; s < ALPHABET && remaining < 0; s++) {
                    if (scaled[s] > 1) {
                        scaled[s]--;
                        remaining++;
                        progress = true;
                    }
                }
                if (!progress) break;
            }
        }
    }

    // Verify sum
    uint32_t sum = 0;
    for (int s = 0; s < ALPHABET; s++) sum += scaled[s];
    if (sum != FREQ_SUM)
        throw std::runtime_error("rANS: frequency scaling failed to reach FREQ_SUM");

    // Build encode table (cumulative starts)
    uint32_t cum = 0;
    for (int s = 0; s < ALPHABET; s++) {
        enc_table_[s].start = cum;
        enc_table_[s].freq  = scaled[s];
        cum += scaled[s];
    }

    scale_bits_ = SCALE_BITS;
    return scaled;
}

// ============================================================================
// Encode
//
// rANS encodes last symbol first. We iterate the data in reverse,
// accumulate bytes into a pre-allocated buffer (filled from the end),
// then return the valid portion.
// ============================================================================

std::vector<uint8_t> RansStaticCoder::encode(const std::vector<uint8_t>& data)
{

    // Worst case: each symbol emits at most 1 renorm byte.
    // +64 for flush bytes + EOF symbol.
    size_t buf_size = data.size() * 2 + 128;
    std::vector<uint8_t> buf(buf_size, 0);

    uint8_t* buf_end = buf.data() + buf_size;
    uint8_t* ptr = buf_end;  // write pointer, moves backward

    RansState rans;
    RansEncInit(&rans);

    // Encode EOF symbol first (it's last in the stream, so encode first in reverse)
    RansEncPut(&rans, &ptr, enc_table_[EOF_SYMBOL].start, enc_table_[EOF_SYMBOL].freq, SCALE_BITS);

    // Encode data in reverse
    for (int i = static_cast<int>(data.size()) - 1; i >= 0; i--) {
        uint8_t sym = data[i];
        RansEncPut(&rans, &ptr, enc_table_[sym].start, enc_table_[sym].freq, SCALE_BITS);
    }

    RansEncFlush(&rans, &ptr);

    // Return only the valid portion (from ptr to buf_end)
    return std::vector<uint8_t>(ptr, buf_end);
}

// ============================================================================
// Build decode table
//
// For each slot in [0, FREQ_SUM), record which symbol owns it plus its
// start/freq. This allows O(1) symbol lookup given cumulative position.
// ============================================================================

void RansStaticCoder::build_decode_table(const std::array<uint16_t, ALPHABET>& scaled_freq)
{
    dec_table_.resize(FREQ_SUM);
    scale_bits_ = SCALE_BITS;

    uint32_t cum = 0;
    for (int s = 0; s < ALPHABET; s++) {
        uint32_t f = scaled_freq[s];
        for (uint32_t slot = cum; slot < cum + f; slot++) {
            dec_table_[slot].sym16  = static_cast<uint16_t>(s);
            dec_table_[slot].symbol = static_cast<uint8_t>(s < 256 ? s : 0);
            dec_table_[slot].start  = cum;
            dec_table_[slot].freq   = f;
        }
        cum += f;
    }
    if (cum != FREQ_SUM)
        throw std::runtime_error("rANS: decode table sum mismatch");
}

// ============================================================================
// Decode
// ============================================================================

std::vector<uint8_t> RansStaticCoder::decode(const std::vector<uint8_t>& rans_stream,
                                               uint64_t original_size)
{
    if (original_size == 0) return {};
    if (rans_stream.size() < 4)
        throw std::runtime_error("rANS: stream too short");

    std::vector<uint8_t> output;
    output.reserve(original_size);

    // Decoder reads forward from the start
    const uint8_t* ptr_start = rans_stream.data();
    uint8_t* ptr = const_cast<uint8_t*>(ptr_start);  // RansDecInit needs non-const*

    RansState rans;
    RansDecInit(&rans, &ptr);

    uint32_t mask = (1u << SCALE_BITS) - 1;

    for (uint64_t i = 0; i < original_size; i++) {
        // Look up symbol
        uint32_t cum = rans & mask;
        const DecEntry& e = dec_table_[cum];
        if (e.sym16 == static_cast<uint16_t>(EOF_SYMBOL)) break;
        output.push_back(e.symbol);
        // Advance state
        RansDecAdvance(&rans, &ptr, e.start, e.freq, SCALE_BITS);
    }

    return output;
}

// ============================================================================
// 2-Way Interleaved Encode
//
// Two independent rANS states alternate symbols (even indices -> state0,
// odd indices -> state1). Both share one output byte stream written backward.
// ~1.4x faster than single-state due to instruction-level parallelism.
// ============================================================================

std::vector<uint8_t> RansStaticCoder::encode_interleaved(const uint8_t* data, size_t len)
{
    if (len == 0) return {};

    size_t buf_size = len * 2 + 128;
    std::vector<uint8_t> buf(buf_size, 0);

    uint8_t* buf_start = buf.data();
    uint8_t* buf_end = buf_start + buf_size;
    uint8_t* ptr = buf_end;

    RansState state0, state1;
    RansEncInit(&state0);
    RansEncInit(&state1);

    // Encode in reverse, alternating states by original data index
    for (int i = static_cast<int>(len) - 1; i >= 0; i--) {
        uint8_t sym = data[i];
        uint32_t start = enc_table_[sym].start;
        uint32_t freq  = enc_table_[sym].freq;

        if ((i & 1) == 0) {
            RansEncPut(&state0, &ptr, start, freq, SCALE_BITS);
        } else {
            RansEncPut(&state1, &ptr, start, freq, SCALE_BITS);
        }
    }

    // Flush state1 first, then state0 (decoder reads state0 first)
    RansEncFlush(&state1, &ptr);
    RansEncFlush(&state0, &ptr);

    return std::vector<uint8_t>(ptr, buf_end);
}

// ============================================================================
// 2-Way Interleaved Decode
//
// Reads state0 then state1 from stream start. Unrolled loop processes
// two symbols per iteration (state0 for even index, state1 for odd).
// Division-free O(1) symbol lookup via dec_table_.
// ============================================================================

size_t RansStaticCoder::decode_interleaved(const uint8_t* rans_stream, size_t stream_len,
                                            uint8_t* output, size_t output_len)
{
    if (output_len == 0) return 0;
    if (stream_len < 8)
        throw std::runtime_error("rANS interleaved: stream too short (need 2x4 bytes for states)");

    uint8_t* ptr = const_cast<uint8_t*>(rans_stream);

    RansState state0, state1;
    RansDecInit(&state0, &ptr);
    RansDecInit(&state1, &ptr);

    uint32_t mask = (1u << SCALE_BITS) - 1;
    size_t pairs = output_len / 2;
    size_t remaining = output_len & 1;
    uint8_t* out = output;

    for (size_t i = 0; i < pairs; i++) {
        // Even index: state0
        uint32_t cum0 = state0 & mask;
        const DecEntry& e0 = dec_table_[cum0];
        *out++ = e0.symbol;
        RansDecAdvance(&state0, &ptr, e0.start, e0.freq, SCALE_BITS);

        // Odd index: state1
        uint32_t cum1 = state1 & mask;
        const DecEntry& e1 = dec_table_[cum1];
        *out++ = e1.symbol;
        RansDecAdvance(&state1, &ptr, e1.start, e1.freq, SCALE_BITS);
    }

    if (remaining) {
        uint32_t cum0 = state0 & mask;
        const DecEntry& e0 = dec_table_[cum0];
        *out++ = e0.symbol;
    }

    return output_len;
}
