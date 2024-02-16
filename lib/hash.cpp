/*
 *  xxHash - Fast Hash algorithm
 *  Copyright (C) 2012-2021, Yann Collet
 *
 *  BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You can contact the author at :
 *  - xxHash homepage: http://www.xxhash.com
 *  - xxHash source repository : https://github.com/Cyan4973/xxHash
 */

#include "hash.h"

#include <cassert>
#include <cstdint>
#include <cstring>

namespace Util {

namespace {
using namespace Detail;

constexpr size_t XXH3_SECRETSIZE_MIN = 136;
constexpr size_t XXH_SECRET_DEFAULT_SIZE = 192;

/* Pseudorandom data taken directly from FARSH */
// clang-format off
constexpr uint8_t kSecret[XXH_SECRET_DEFAULT_SIZE] = {
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
    0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
    0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
    0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb, 0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
    0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
    0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31, 0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
    0xea, 0xc5, 0xac, 0x83, 0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e,
    0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce,
    0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
};
// clang-format on

#if defined(_WIN32) /* Windows is always little endian */ \
    || defined(__LITTLE_ENDIAN__) ||                      \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define XXH_CPU_LITTLE_ENDIAN 1
#elif defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define XXH_CPU_LITTLE_ENDIAN 0
#else
static int XXH_isLittleEndian(void) {
    const union {
        uint32_t u;
        uint8_t c[4];
    } one = {1};
    return one.c[0];
}
#define XXH_CPU_LITTLE_ENDIAN XXH_isLittleEndian()
#endif

inline __attribute__((always_inline)) static uint32_t XXH_read32(const void *memPtr) {
    uint32_t val;
    memcpy(&val, memPtr, sizeof(val));
    return val;
}

inline __attribute__((always_inline)) static uint64_t XXH_read64(const void *memPtr) {
    uint64_t val;
    memcpy(&val, memPtr, sizeof(val));
    return val;
}

#if defined(_MSC_VER) /* Visual Studio */
#define XXH_swap32 _byteswap_ulong
#elif defined(__GNUC__)
#define XXH_swap32 __builtin_bswap32
#else
static uint32_t XXH_swap32(uint32_t x) {
    return ((x << 24) & 0xff000000) | ((x << 8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) |
           ((x >> 24) & 0x000000ff);
}
#endif

#if defined(_MSC_VER) /* Visual Studio */
#define XXH_swap64 _byteswap_uint64
#elif defined(__GNUC__)
#define XXH_swap64 __builtin_bswap64
#else
static uint32_t XXH_swap64(uint32_t x) {
    return ((x << 56) & UINT64_C(0xff00000000000000)) | ((x << 40) & UINT64_C(0x00ff000000000000)) |
           ((x << 24) & UINT64_C(0x0000ff0000000000)) | ((x << 8) & UINT64_C(0x000000ff00000000)) |
           ((x >> 8) & UINT64_C(0x00000000ff000000)) | ((x >> 24) & UINT64_C(0x0000000000ff0000)) |
           ((x >> 40) & UINT64_C(0x000000000000ff00)) | ((x >> 56) & UINT64_C(0x00000000000000ff));
}
#endif

#if (defined(__GNUC__) && (__GNUC__ >= 3)) || \
    (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800)) || defined(__clang__)
#define XXH_likely(x) __builtin_expect(x, 1)
#define XXH_unlikely(x) __builtin_expect(x, 0)
#else
#define XXH_likely(x) (x)
#define XXH_unlikely(x) (x)
#endif

inline __attribute__((always_inline)) static uint32_t XXH_readLE32(const void *ptr) {
    return XXH_CPU_LITTLE_ENDIAN ? XXH_read32(ptr) : XXH_swap32(XXH_read32(ptr));
}

inline __attribute__((always_inline)) static uint64_t XXH_readLE64(const void *ptr) {
    return XXH_CPU_LITTLE_ENDIAN ? XXH_read64(ptr) : XXH_swap64(XXH_read64(ptr));
}

// Calculates a 64-bit to 128-bit multiply, then XOR folds it.
static uint64_t XXH3_mul128_fold64(uint64_t lhs, uint64_t rhs) {
#if defined(__SIZEOF_INT128__) || (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 128)
    __uint128_t product = (__uint128_t)lhs * (__uint128_t)rhs;
    return uint64_t(product) ^ uint64_t(product >> 64);

#else
    /* First calculate all of the cross products. */
    const uint64_t lo_lo = (lhs & 0xFFFFFFFF) * (rhs & 0xFFFFFFFF);
    const uint64_t hi_lo = (lhs >> 32) * (rhs & 0xFFFFFFFF);
    const uint64_t lo_hi = (lhs & 0xFFFFFFFF) * (rhs >> 32);
    const uint64_t hi_hi = (lhs >> 32) * (rhs >> 32);

    /* Now add the products together. These will never overflow. */
    const uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    const uint64_t upper = (hi_lo >> 32) + (cross >> 32) + hi_hi;
    const uint64_t lower = (cross << 32) | (lo_lo & 0xFFFFFFFF);

    return upper ^ lower;
#endif
}

constexpr size_t XXH_STRIPE_LEN = 64;
constexpr size_t XXH_SECRET_CONSUME_RATE = 8;
constexpr size_t XXH_ACC_NB = XXH_STRIPE_LEN / sizeof(uint64_t);

static uint64_t XXH3_avalanche(uint64_t hash) {
    hash ^= hash >> 37;
    hash *= PRIME_MX1;
    hash ^= hash >> 32;
    return hash;
}

static uint64_t XXH3_len_1to3_64b(const uint8_t *input, size_t len, const uint8_t *secret,
                                  uint64_t seed) {
    const uint8_t c1 = input[0];
    const uint8_t c2 = input[len >> 1];
    const uint8_t c3 = input[len - 1];
    uint32_t combined =
        ((uint32_t)c1 << 16) | ((uint32_t)c2 << 24) | ((uint32_t)c3 << 0) | ((uint32_t)len << 8);
    uint64_t bitflip = (uint64_t)(XXH_readLE32(secret) ^ XXH_readLE32(secret + 4)) + seed;
    return XXH64_avalanche(uint64_t(combined) ^ bitflip);
}

static uint64_t XXH3_len_4to8_64b(const uint8_t *input, size_t len, const uint8_t *secret,
                                  uint64_t seed) {
    seed ^= (uint64_t)XXH_swap32(uint32_t(seed)) << 32;
    const uint32_t input1 = XXH_readLE32(input);
    const uint32_t input2 = XXH_readLE32(input + len - 4);
    uint64_t acc = (XXH_readLE64(secret + 8) ^ XXH_readLE64(secret + 16)) - seed;
    const uint64_t input64 = (uint64_t)input2 | ((uint64_t)input1 << 32);
    acc ^= input64;
    // XXH3_rrmxmx(acc, len)
    acc ^= rotl64(acc, 49) ^ rotl64(acc, 24);
    acc *= PRIME_MX2;
    acc ^= (acc >> 35) + (uint64_t)len;
    acc *= PRIME_MX2;
    return acc ^ (acc >> 28);
}

static uint64_t XXH3_len_9to16_64b(const uint8_t *input, size_t len, const uint8_t *secret,
                                   uint64_t const seed) {
    uint64_t input_lo = (XXH_readLE64(secret + 24) ^ XXH_readLE64(secret + 32)) + seed;
    uint64_t input_hi = (XXH_readLE64(secret + 40) ^ XXH_readLE64(secret + 48)) - seed;
    input_lo ^= XXH_readLE64(input);
    input_hi ^= XXH_readLE64(input + len - 8);
    uint64_t acc =
        uint64_t(len) + XXH_swap64(input_lo) + input_hi + XXH3_mul128_fold64(input_lo, input_hi);
    return XXH3_avalanche(acc);
}

inline __attribute__((always_inline)) static uint64_t XXH3_len_0to16_64b(const uint8_t *input,
                                                                         size_t len,
                                                                         const uint8_t *secret,
                                                                         uint64_t const seed) {
    if (XXH_likely(len > 8)) return XXH3_len_9to16_64b(input, len, secret, seed);
    if (XXH_likely(len >= 4)) return XXH3_len_4to8_64b(input, len, secret, seed);
    if (len != 0) return XXH3_len_1to3_64b(input, len, secret, seed);
    return XXH64_avalanche(seed ^ XXH_readLE64(secret + 56) ^ XXH_readLE64(secret + 64));
}

static uint64_t XXH3_mix16B(const uint8_t *input, uint8_t const *secret, uint64_t seed) {
    uint64_t lhs = seed;
    uint64_t rhs = 0U - seed;
    lhs += XXH_readLE64(secret);
    rhs += XXH_readLE64(secret + 8);
    lhs ^= XXH_readLE64(input);
    rhs ^= XXH_readLE64(input + 8);
    return XXH3_mul128_fold64(lhs, rhs);
}

/* For mid range keys, XXH3 uses a Mum-hash variant. */
inline __attribute__((always_inline)) static uint64_t XXH3_len_17to128_64b(const uint8_t *input,
                                                                           size_t len,
                                                                           const uint8_t *secret,
                                                                           uint64_t const seed) {
    uint64_t acc = len * PRIME64_1, acc_end;
    acc += XXH3_mix16B(input + 0, secret + 0, seed);
    acc_end = XXH3_mix16B(input + len - 16, secret + 16, seed);
    if (len > 32) {
        acc += XXH3_mix16B(input + 16, secret + 32, seed);
        acc_end += XXH3_mix16B(input + len - 32, secret + 48, seed);
        if (len > 64) {
            acc += XXH3_mix16B(input + 32, secret + 64, seed);
            acc_end += XXH3_mix16B(input + len - 48, secret + 80, seed);
            if (len > 96) {
                acc += XXH3_mix16B(input + 48, secret + 96, seed);
                acc_end += XXH3_mix16B(input + len - 64, secret + 112, seed);
            }
        }
    }
    return XXH3_avalanche(acc + acc_end);
}

constexpr size_t XXH3_MIDSIZE_MAX = 240;

__attribute__((noinline)) static uint64_t XXH3_len_129to240_64b(const uint8_t *input, size_t len,
                                                                const uint8_t *secret,
                                                                uint64_t seed) {
    constexpr size_t XXH3_MIDSIZE_STARTOFFSET = 3;
    constexpr size_t XXH3_MIDSIZE_LASTOFFSET = 17;
    uint64_t acc = (uint64_t)len * PRIME64_1;
    const unsigned nbRounds = len / 16;
    for (unsigned i = 0; i < 8; ++i) acc += XXH3_mix16B(input + 16 * i, secret + 16 * i, seed);
    acc = XXH3_avalanche(acc);

    for (unsigned i = 8; i < nbRounds; ++i) {
        acc += XXH3_mix16B(input + 16 * i, secret + 16 * (i - 8) + XXH3_MIDSIZE_STARTOFFSET, seed);
    }
    /* last bytes */
    acc +=
        XXH3_mix16B(input + len - 16, secret + XXH3_SECRETSIZE_MIN - XXH3_MIDSIZE_LASTOFFSET, seed);
    return XXH3_avalanche(acc);
}

inline __attribute__((always_inline)) static void XXH3_accumulate_512_scalar(
    uint64_t *acc, const uint8_t *input, const uint8_t *secret) {
    for (size_t i = 0; i < XXH_ACC_NB; ++i) {
        uint64_t data_val = XXH_readLE64(input + 8 * i);
        uint64_t data_key = data_val ^ XXH_readLE64(secret + 8 * i);
        acc[i ^ 1] += data_val;
        acc[i] += uint32_t(data_key) * (data_key >> 32);
    }
}

inline __attribute__((always_inline)) static void XXH3_accumulate_scalar(uint64_t *acc,
                                                                         const uint8_t *input,
                                                                         const uint8_t *secret,
                                                                         size_t nbStripes) {
    for (size_t n = 0; n < nbStripes; ++n)
        XXH3_accumulate_512_scalar(acc, input + n * XXH_STRIPE_LEN,
                                   secret + n * XXH_SECRET_CONSUME_RATE);
}

static void XXH3_scrambleAcc(uint64_t *acc, const uint8_t *secret) {
    for (size_t i = 0; i < XXH_ACC_NB; ++i) {
        acc[i] ^= acc[i] >> 47;
        acc[i] ^= XXH_readLE64(secret + 8 * i);
        acc[i] *= PRIME32_1;
    }
}

static uint64_t XXH3_mix2Accs(const uint64_t *acc, const uint8_t *secret) {
    return XXH3_mul128_fold64(acc[0] ^ XXH_readLE64(secret), acc[1] ^ XXH_readLE64(secret + 8));
}

static uint64_t XXH3_mergeAccs(const uint64_t *acc, const uint8_t *key, uint64_t start) {
    uint64_t result64 = start;
    for (size_t i = 0; i < 4; ++i) result64 += XXH3_mix2Accs(acc + 2 * i, key + 16 * i);
    return XXH3_avalanche(result64);
}

__attribute__((noinline)) static uint64_t XXH3_hashLong_64b(const uint8_t *input, size_t len,
                                                            const uint8_t *secret,
                                                            size_t secretSize) {
    const size_t nbStripesPerBlock = (secretSize - XXH_STRIPE_LEN) / XXH_SECRET_CONSUME_RATE;
    const size_t block_len = XXH_STRIPE_LEN * nbStripesPerBlock;
    const size_t nb_blocks = (len - 1) / block_len;
    alignas(16) uint64_t acc[XXH_ACC_NB] = {
        PRIME32_3, PRIME64_1, PRIME64_2, PRIME64_3, PRIME64_4, PRIME32_2, PRIME64_5, PRIME32_1,
    };
    for (size_t n = 0; n < nb_blocks; ++n) {
        XXH3_accumulate_scalar(acc, input + n * block_len, secret, nbStripesPerBlock);
        XXH3_scrambleAcc(acc, secret + secretSize - XXH_STRIPE_LEN);
    }

    /* last partial block */
    const size_t nbStripes = (len - 1 - (block_len * nb_blocks)) / XXH_STRIPE_LEN;
    assert(nbStripes <= secretSize / XXH_SECRET_CONSUME_RATE);
    XXH3_accumulate_scalar(acc, input + nb_blocks * block_len, secret, nbStripes);

    /* last stripe */
    constexpr size_t XXH_SECRET_LASTACC_START = 7;
    XXH3_accumulate_512_scalar(acc, input + len - XXH_STRIPE_LEN,
                               secret + secretSize - XXH_STRIPE_LEN - XXH_SECRET_LASTACC_START);

    /* converge into final hash */
    constexpr size_t XXH_SECRET_MERGEACCS_START = 11;
    return XXH3_mergeAccs(acc, secret + XXH_SECRET_MERGEACCS_START, (uint64_t)len * PRIME64_1);
}
}  // namespace

uint64_t hash(const void *in, size_t len) {
    const uint8_t *data = reinterpret_cast<const uint8_t *>(in);
    if (len <= 16) return XXH3_len_0to16_64b(data, len, kSecret, 0);
    if (len <= 128) return XXH3_len_17to128_64b(data, len, kSecret, 0);
    if (len <= XXH3_MIDSIZE_MAX) return XXH3_len_129to240_64b(data, len, kSecret, 0);
    return XXH3_hashLong_64b(data, len, kSecret, sizeof(kSecret));
}
}  // namespace Util
