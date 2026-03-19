// rans_byte.h - ryg's rANS byte-stream implementation
// From: https://github.com/rygorous/ryg_rans
// License: Public Domain (CC0 / Unlicense)
// Taken from the original file verbatim; only this comment header was added.
//
// rANS (range Asymmetric Numeral Systems) with byte-granularity I/O.
// Encodes symbols in REVERSE: encoder processes last symbol first.
// Decoder processes forward.
//
// Usage:
//   Encoder: fill buf[] from end; call RansEncPutSymbol() for each symbol (last first);
//            then flush with RansEncFlush() and output bytes from put_ptr to end.
//   Decoder: init with RansDecInit(); call RansDecGetSymbol() per symbol.

#pragma once
#include <stdint.h>
#include <assert.h>

// --------------------------------------------------------------------------

// L = lower bound of normalisation interval
#define RANS_BYTE_L (1u << 23)  // 8-bit renorm, lower bound

// State type (must be at least 32 bits)
typedef uint32_t RansState;

// --------------------------------------------------------------------------
// Encoder

static inline void RansEncInit(RansState* r)
{
    *r = RANS_BYTE_L;
}

static inline void RansEncFlush(RansState* r, uint8_t** pptr)
{
    RansState x = *r;
    uint8_t* ptr = *pptr;
    ptr -= 4;
    ptr[0] = (uint8_t)(x >> 0);
    ptr[1] = (uint8_t)(x >> 8);
    ptr[2] = (uint8_t)(x >> 16);
    ptr[3] = (uint8_t)(x >> 24);
    *pptr = ptr;
}

static inline void RansEncInit64(uint64_t* r)
{
    *r = RANS_BYTE_L;
}

// Encode a symbol with cum_freq in [0, freq_sum) with frequency freq.
// scale_bits = log2(freq_sum); freq_sum must be a power of 2.
static inline void RansEncPut(RansState* r, uint8_t** pptr,
                               uint32_t start, uint32_t freq, uint32_t scale_bits)
{
    assert(freq != 0);
    // x = C(s, x)
    RansState x = *r;
    // renormalise
    uint32_t x_max = ((RANS_BYTE_L >> scale_bits) << 8) * freq; // = freq << (23-scale_bits+8)
    while (x >= x_max) {
        *--(*pptr) = (uint8_t)(x & 0xff);
        x >>= 8;
    }
    // encode
    *r = ((x / freq) << scale_bits) + (x % freq) + start;
}

// --------------------------------------------------------------------------
// Decoder

static inline void RansDecInit(RansState* r, uint8_t** pptr)
{
    uint32_t x;
    uint8_t* ptr = *pptr;
    x  = ptr[0] << 0;
    x |= ptr[1] << 8;
    x |= ptr[2] << 16;
    x |= ptr[3] << 24;
    *pptr = ptr + 4;
    *r = x;
}

static inline uint32_t RansDecGet(RansState* r, uint32_t scale_bits)
{
    return *r & ((1u << scale_bits) - 1);
}

static inline void RansDecAdvance(RansState* r, uint8_t** pptr,
                                   uint32_t start, uint32_t freq, uint32_t scale_bits)
{
    uint32_t mask = (1u << scale_bits) - 1;
    // s, x = D(x)
    RansState x = *r;
    x = freq * (x >> scale_bits) + (x & mask) - start;
    // renormalise
    while (x < RANS_BYTE_L) {
        x = (x << 8) | *(*pptr)++;
    }
    *r = x;
}

// --------------------------------------------------------------------------
// Symbol table (compact encode / decode helpers used by the static model)

struct RansSymbol {
    uint32_t start;  // cumulative frequency
    uint32_t freq;   // frequency
};

static inline void RansEncPutSymbol(RansState* r, uint8_t** pptr,
                                     const RansSymbol* sym, uint32_t scale_bits)
{
    RansEncPut(r, pptr, sym->start, sym->freq, scale_bits);
}

static inline uint32_t RansDecGetSymbol(RansState* r, uint8_t** pptr,
                                         const RansSymbol* sym, uint32_t scale_bits)
{
    // caller already looked up the symbol from RansDecGet(); just advance
    RansDecAdvance(r, pptr, sym->start, sym->freq, scale_bits);
    return sym->freq; // return value unused typically
}
