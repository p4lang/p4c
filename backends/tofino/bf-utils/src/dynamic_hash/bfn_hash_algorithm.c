#include "bf-utils/include/dynamic_hash/bfn_hash_algorithm.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * These values are based on the crc calculations provided at this url:
 *     http://reveng.sourceforge.net/crc-catalogue/all.htm
 *
 * It is understood that the top bit of the the polynomial is known to be == 1,
 * but any polynomial over 64 bits would not fit.  Thus the strings do not have
 * this value
 */
static crc_alg_info_t standard_crc_algs[CRC_INVALID] = {
    {"crc_8", 0x07, false, 0, 0},
    {"crc_8_darc", 0x39, true, 0, 0},
    {"crc_8_i_code", 0x1d, false, 0xfd, 0},
    {"crc_8_itu", 0x07, false, 0x55, 0x55},
    {"crc_8_maxim", 0x31, true, 0, 0},
    {"crc_8_rohc", 0x07, true, 0xff, 0},
    {"crc_8_wcdma", 0x9b, true, 0, 0},
    {"crc_16", 0x8005, true, 0, 0},
    {"crc_16_bypass", 0x8005, false, 0, 0},
    {"crc_16_dds_110", 0x8005, false, 0x800d, 0},
    // This one doesn't appear on the website, but kept to not break
    // regressions
    {"crc_16_dect", 0x0589, false, 1, 1},
    {"crc_16_dect_r", 0x0589, false, 0, 1},
    {"crc_16_dect_x", 0x0589, false, 0, 0},
    {"crc_16_dnp", 0x3d65, true, 0, 0xffff},
    {"crc_16_en_13757", 0x3d65, false, 0, 0xffff},
    {"crc_16_genibus", 0x1021, false, 0xffff, 0xffff},
    {"crc_16_maxim", 0x8005, true, 0, 0xffff},
    {"crc_16_mcrf4xx", 0x1021, true, 0xffff, 0},
    {"crc_16_riello", 0x1021, true, 0xb2aa, 0},
    {"crc_16_t10_dif", 0x8bb7, false, 0, 0},
    {"crc_16_teledisk", 0xa097, false, 0, 0},
    {"crc_16_usb", 0x8005, true, 0xffff, 0xffff},
    {"x_25", 0x1021, true, 0xffff, 0xffff},
    {"xmodem", 0x1021, false, 0, 0},
    {"modbus", 0x8005, true, 0xffff, 0},
    {"kermit", 0x1021, true, 0, 0},
    {"crc_ccitt_false", 0x1021, false, 0xffff, 0},
    {"crc_aug_ccitt", 0x1021, false, 0x1d0f, 0},
    {"crc_32", 0x04c11db7, true, 0xffffffff, 0xffffffff},
    {"crc_32_bzip2", 0x04c11db7, false, 0xffffffff, 0xffffffff},
    {"crc_32c", 0x1edc6f41, true, 0xffffffff, 0xffffffff},
    {"crc_32d", 0xa833982b, true, 0xffffffff, 0xffffffff},
    {"crc_32_mpeg", 0x04c11db7, false, 0xffffffff, 0},
    {"posix", 0x04c11db7, false, 0, 0xffffffff},
    {"crc_32q", 0x814141ab, false, 0, 0},
    {"jamcrc", 0x04c11db7, true, 0xffffffff, 0},
    {"xfer", 0x000000af, false, 0, 0},
    {"crc_64", 0x42f0e1eba9ea3693, false, 0, 0},
    {"crc_64_go_iso", 0x000000000000001b, true, 0xffffffffffffffff, 0xffffffffffffffff},
    {"crc_64_we", 0x42f0e1eba9ea3693, false, 0xffffffffffffffff, 0xffffffffffffffff},
    // This one doesn't appear on the website, but kept to not break
    // regressions
    {"crc_64_jones", 0xad93d23594c935a9, true, 0xffffffffffffffff, 0}};

int determine_msb(uint64_t value) {
    int rv = -1;
#if defined(__GNUC__) || defined(__clang__)
    if (value) rv = 63 - __builtin_clzl(value);
#else
    for (int i = 63; i >= 0; i--) {
        if (((1ULL << i) & value) != 0ULL) return i;
    }
#endif
    return rv;
}

#define BUILD_CRC_ERRORS

static char *bfn_crc_errors[BUILD_CRC_ERRORS] = {
    "Polynomial is not odd",
    "Init value is larger than the polynomial",
    "Final xor value is larger than the polynomial",
};

bool verify_algorithm(bfn_hash_algorithm_t *alg, char **error_message) {
    if ((alg->poly & 1) == 0) {
        if (error_message) *error_message = bfn_crc_errors[0];
        return false;
    }
    if (alg->init && determine_msb(alg->init) + 1 > alg->hash_bit_width) {
        if (error_message) *error_message = bfn_crc_errors[1];
        return false;
    }
    if (alg->final_xor && determine_msb(alg->final_xor) + 1 > alg->hash_bit_width) {
        if (error_message) *error_message = bfn_crc_errors[2];
        return false;
    }
    return true;
}

/**
 * Builds the algorithm from the standard crc algorithm position
 */
void build_standard_crc_algorithm(bfn_hash_algorithm_t *alg, bfn_crc_alg_t crc_alg) {
    if (crc_alg >= CRC_8 && crc_alg <= CRC_8_WCDMA)
        alg->hash_bit_width = 8;
    else if (crc_alg >= CRC_16 && crc_alg <= CRC_AUG_CCITT)
        alg->hash_bit_width = 16;
    else if (crc_alg >= CRC_32 && crc_alg <= XFER)
        alg->hash_bit_width = 32;
    else if (crc_alg >= CRC_64 && crc_alg <= CRC_64_JONES)
        alg->hash_bit_width = 64;

    crc_alg_info_t crc_alg_info = standard_crc_algs[(int)crc_alg];
    alg->poly = crc_alg_info.poly;
    alg->reverse = crc_alg_info.reverse;
    alg->init = crc_alg_info.init;
    alg->final_xor = crc_alg_info.final_xor;
    alg->crc_type = crc_alg_str_to_type(crc_alg_info.crc_name);
}

void initialize_algorithm(bfn_hash_algorithm_t *alg, bfn_hash_alg_type_t hash_alg, bool msb,
                          bool extend, bfn_crc_alg_t crc_alg) {
    alg->hash_alg = hash_alg;
    alg->msb = msb;
    alg->extend = extend;
    if (crc_alg != CRC_INVALID) build_standard_crc_algorithm(alg, crc_alg);
}

static uint64_t invert_poly(uint64_t poly, int bit_width) {
    uint64_t ret = 0, i = 0;
    while (poly > 0) {
        ret |= ((poly & 0x1) << (bit_width - i - 1));
        i++;
        poly >>= 1;
    }
    return ret;
}

static void construct_stream(bfn_hash_algorithm_t *alg, uint64_t val, uint8_t *stream) {
    uint8_t width = (alg->hash_bit_width + 7) / 8;
    for (uint8_t i = 0; i < width; i++) {
        stream[width - i - 1] = (val >> (i * 8));
    }
    return;
}

static void shift_right(uint8_t *stream, uint8_t bit_width) {
    uint8_t width = (bit_width + 7) / 8;
    for (int i = width - 1; i >= 0; i--) {
        stream[i] >>= 1;
        if (i > 0) {
            stream[i] |= (stream[i - 1] & 0x1) << 7;
        }
    }
}

static void shift_left(uint8_t *stream, uint8_t bit_width) {
    uint8_t width = (bit_width + 7) / 8;
    for (uint8_t i = 0; i < width; i++) {
        stream[i] <<= 1;
        if (i < width - 1) {
            stream[i] |= (stream[i + 1] >> 7) & 0x1;
        }
    }
}

void initialize_crc_matrix(bfn_hash_algorithm_t *alg) {
    if (alg->hash_alg != CRC_DYN) {
        return;
    }

    uint32_t width = (alg->hash_bit_width + 7) / 8, i = 0;
    uint8_t poly[width], rem[width];
    memset(poly, 0, width * sizeof(uint8_t));
    memset(rem, 0, width * sizeof(uint8_t));

    uint64_t poly_val = alg->poly;
    if (alg->reverse) {
        poly_val = invert_poly(poly_val, alg->hash_bit_width);
    }
    construct_stream(alg, poly_val, poly);
    uint32_t byte = 0, bit = 0;
    bool xor = false;
    if (alg->reverse) {
        for (byte = 0; byte < 256; byte++) {
            memset(rem, 0, width * sizeof(uint8_t));
            rem[width - 1] = byte;
            for (bit = 0; bit < 8; bit++) {
                xor = rem[width - 1] & 0x1;
                shift_right(rem, alg->hash_bit_width);
                if (xor) {
                    for (i = 0; i < width; i++) {
                        rem[i] ^= poly[i];
                    }
                }
            }
            memcpy(alg->crc_matrix[byte], rem, width);
        }
    } else {
        uint8_t offset = alg->hash_bit_width % 8;
        uint8_t top_bit = offset == 0 ? 0x80 : 1 << ((offset - 1) % 8);
        uint8_t top_mask = top_bit == 0x80 ? 0xff : (top_bit << 1) - 1;
        for (byte = 0; byte < 256; byte++) {
            memset(rem, 0, width * sizeof(uint8_t));
            if (offset == 0) {
                rem[0] = byte;
            } else {
                rem[0] = byte >> (8 - offset);
                rem[1] = byte << offset;
            }
            for (bit = 0; bit < 8; bit++) {
                xor = rem[0] & top_bit;
                shift_left(rem, alg->hash_bit_width);
                if (xor) {
                    for (i = 0; i < width; i++) {
                        rem[i] ^= poly[i];
                    }
                }
                rem[0] &= top_mask;
            }
            memcpy(alg->crc_matrix[byte], rem, width);
        }
    }

    return;
}

static uint8_t get_byte(uint8_t *stream, uint8_t index, uint8_t offset) {
    if (offset == 0) {
        return stream[index];
    }
    return ((stream[index] << (8 - offset)) | (stream[index + 1] >> offset));
}

void calculate_crc(bfn_hash_algorithm_t *alg, uint32_t hash_output_bits, uint8_t *stream,
                   uint32_t stream_len, uint8_t *crc) {
    uint32_t i = 0;
    uint8_t idx = 0;
    uint8_t width = (alg->hash_bit_width + 7) / 8;
    uint8_t hash_output_bytes = (hash_output_bits + 7) / 8;
    uint8_t final_xor[width];
    memset(crc, 0, hash_output_bytes * sizeof(uint8_t));
    memset(final_xor, 0, width * sizeof(uint8_t));
    construct_stream(alg, alg->init, crc);
    construct_stream(alg, alg->final_xor, final_xor);
    uint8_t *crc_str = crc + hash_output_bytes - width;

    for (i = 0; i < stream_len; i++) {
        if (alg->reverse) {
            idx = crc_str[width - 1] ^ stream[i];
            for (int j = width - 1; j > 0; j--) {
                crc_str[j] = crc_str[j - 1] ^ alg->crc_matrix[idx][j];
            }
            crc_str[0] = alg->crc_matrix[idx][0];
        } else {
            uint8_t offset = alg->hash_bit_width % 8;
            uint8_t mask = offset == 0 ? 0xff : (1 << offset) - 1;
            idx = get_byte(crc_str, 0, offset) ^ stream[i];
            for (uint8_t j = 0; j < width - 1; j++) {
                crc_str[j] = (crc_str[j + 1] ^ alg->crc_matrix[idx][j]) & mask;
                mask = 0xff;
            }
            crc_str[width - 1] = alg->crc_matrix[idx][width - 1];
        }
    }

    for (i = 0; i < width; i++) {
        crc_str[i] ^= final_xor[i];
    }

    if (alg->extend) {
        for (i = 0; i < (uint32_t)(hash_output_bytes - width); i++) {
            *(crc_str - i - 1) = crc_str[width - i % width - 1];
        }
        if (hash_output_bits % 8) {
            crc[0] &= (1 << (hash_output_bits % 8)) - 1;
        }
    }

    return;
}
