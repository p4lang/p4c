#include "bf-utils/include/dynamic_hash/dynamic_hash.h"

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define assert_macro assert
#define calloc_macro calloc
#define free_macro free

#define ALLOC_CHECK(ptr, str) \
  do {                        \
    if (ptr == NULL) {        \
      printf("%s\n", str);    \
      assert_macro(0);        \
      return;                 \
    }                         \
  } while (0)

bool determine_identity_gfm(uint32_t input_bit, uint32_t p4_hash_output_bit) {
  return input_bit == p4_hash_output_bit;
}

bool determine_random_gfm() { return random() & 1; }

static bool determine_xor_gfm(uint32_t input_bit,
                              uint32_t p4_hash_output_bit,
                              uint32_t width,
                              uint32_t total_input_bits) {
  /* -- The message (input registers) is handled as a big integer - the
   *    highest bit of the integer is the beginning of the message. Zero-th
   *    bit is the end of the message. If the message is not aligned with
   *    the algorithm's width, zero bits must be appended after the end
   *    of the message - that's the reason of shifting the input bit
   *    by total_input_bits. */
  return (p4_hash_output_bit < width) &&
         ((input_bit + width * width - total_input_bits) % width) ==
             p4_hash_output_bit;
}

/**
 * The point of this funciton is to get up to a 64 bit value from the
 * bitvector.
 *
 * This is potientially broken up into two separate lookups, if the boundary
 * of the write crosses a 64 bit section.  The bitvec is an array of 64 bit
 * values.  Thus if the algorithm wanted to get a 16 bit value at position
 * 56, the value returned would be the upper 8 bits of bitvec[0] starting at
 * bit 0, ORed with the lower 8 bits of bitvec[1] starting at bit 8.
 *
 * The lower_* values in the previous example would deal with any reads
 * to bitvec[0], while all upper_* values deal with any reads to bitvec[1]
 */
uint64_t get_from_bitvec(const uint64_t *val,
                         uint32_t bitvec_sz,
                         uint32_t start_bit,
                         uint32_t size) {
  uint64_t current_value = 0ULL;
  uint32_t mod_pos = start_bit % 64;
  uint32_t bitvec_pos = start_bit / 64;
  uint64_t upper_mask = (1ULL << mod_pos) - 1;
  uint64_t lower_mask = ULLONG_MAX & ~upper_mask;

  uint64_t upper_value = 0ULL;
  if (mod_pos && bitvec_pos + 1 < bitvec_sz) {
    upper_value = upper_mask & *(val + bitvec_pos + 1);
    upper_value <<= 64 - mod_pos;
  }

  uint64_t lower_value = lower_mask & *(val + bitvec_pos);
  lower_value >>= mod_pos;
  current_value = lower_value | upper_value;

  uint64_t mask = 0ULL;
  if (size >= 64ULL) {
    mask = ULLONG_MAX;
  } else {
    mask = (1ULL << size) - 1;
  }

  return current_value & mask;
}

/**
 * The point of this function is to write up to a 64 bit value into a position
 * of the bitvector.
 *
 * This is potentially broken up into two separate assignment, if the boundary
 * of the write crosses a 64 bit section.  The bitvec is an array of 64 bit
 * values.  Thus if the algorithm wanted to write a 16 bit value into the
 * bitvec at position 56, bitvec[0] upper 8 bits would be written and
 * bitvec[1] lower 8 bits would be written.
 *
 * The lower_* values in the previous example would deal with any writes
 * to bitvec[0], while all upper_* values deal with any writes to bitvec[1]
 */
void set_into_bitvec(uint64_t *val,
                     uint32_t bitvec_sz,
                     uint64_t current_value,
                     uint32_t start_bit,
                     uint32_t size) {
  uint64_t mask = 0ULL;
  if (size >= 64ULL) {
    mask = ULLONG_MAX;
  } else {
    mask = (1ULL << size) - 1;
  }
  uint32_t mod_pos = start_bit % 64;
  uint32_t bitvec_pos = start_bit / 64;

  // Bits of the current_value headed to the lower bitvec position
  uint64_t lower_mask = ULLONG_MAX;
  if (mod_pos != 0) lower_mask = (1ULL << (64 - mod_pos)) - 1;
  // Bits of the current_value headed to the upper bitvec position
  uint64_t upper_mask = ULLONG_MAX & ~lower_mask;
  uint64_t upper_value = upper_mask & current_value & mask;
  if (mod_pos)
    upper_value >>= 64 - mod_pos;
  else
    upper_value = 0;

  // Bits of the upper bitvecs that will be written into
  uint64_t upper_set_mask = ULLONG_MAX;
  if (mod_pos != 0) upper_set_mask = ~(mask >> (64 - mod_pos)) & ULLONG_MAX;

  uint64_t lower_value = lower_mask & current_value & mask;
  lower_value <<= mod_pos;
  // Bits of the lower bitvecs that will be written into
  uint64_t lower_set_mask = ~(mask << mod_pos) & ULLONG_MAX;

  if (bitvec_pos + 1 < bitvec_sz) {
    *(val + bitvec_pos + 1) =
        upper_value | (upper_set_mask & *(val + bitvec_pos + 1));
  }
  *(val + bitvec_pos) = lower_value | (lower_set_mask & *(val + bitvec_pos));
}

uint32_t compute_bitvec_sz(const bfn_hash_algorithm_t *alg,
                           uint32_t total_input_bits) {
  uint32_t total_msb = (((total_input_bits - 1) + 7) / 8) * 8;
  if (alg->hash_alg == CRC_DYN) total_msb += alg->hash_bit_width;
  return (total_msb + 63) / 64;
}

/**
 * The constants from the hash field inputs need to be correctly added to
 * the initial bit vector.  This function writes these constants into
 * the bit vector at the correct location.
 *
 * The purpose of this library is to determine each relevant hash matrix bit
 * which has a corresponding input and output bit.  All input bits correspond
 * to a PHV bit, as that is the input to the hash matrix.  For constant inputs,
 * these constants have no single input xbar bit, and thus have no single row
 * in the hash matrix that they coordinate to.  However, in order to calculate
 * each individual row's matrix, the constants must be included in every
 * calculation of that data.
 */
void initialize_input_data_stream(const bfn_hash_algorithm_t *alg,
                                  uint64_t *val,
                                  uint32_t bitvec_sz,
                                  const ixbar_input_t *inputs,
                                  uint32_t inputs_sz) {
  int start_bit = 0;
  if (alg->hash_alg == CRC_DYN) start_bit += alg->hash_bit_width;
  for (uint32_t i = 0; i < inputs_sz; i++) {
    const ixbar_input_t *input = inputs + i;
    if (input->type == tCONST) {
      // For reverse algorithms, the inputs are byte reversed.  This is
      // true for any constant input as well.
      if (alg->hash_alg == CRC_DYN && alg->reverse) {
        for (uint32_t j = 0; j < input->bit_size; j++) {
          uint32_t val_bit_pos = start_bit + j;
          val_bit_pos -= alg->hash_bit_width;
          val_bit_pos ^= 7;
          val_bit_pos += alg->hash_bit_width;
          uint64_t val_bit_val = (input->u.constant >> j) & 1ULL;
          set_into_bitvec(val, bitvec_sz, val_bit_val, val_bit_pos, 1);
        }

        // All other algorithms have the constants in order, so we add them
        // in order
      } else {
        uint32_t val_bit_pos = start_bit;
        uint32_t val_bit_len = input->bit_size;
        uint64_t val_bit_val = input->u.constant;
        set_into_bitvec(val, bitvec_sz, val_bit_val, val_bit_pos, val_bit_len);
      }
    }
    start_bit += input->bit_size;
  }
}

/**
 * The matrix position i, j where:
 *
 *     i - the input bit of the data stream
 *     j - the output bit of the hash function
 *
 * will be equal to whether:
 *     (1 << output_bit) & crc(alg, data_stream)) == (1 << output),
 *
 * where the data stream is an all 0 bitstream except for a single bit at the
 * input_bit position.
 *
 * The output of the crc is the remainder of a polynomial division modulo two
 * of a data stream with a polynomial.  From msb to lsb, the polynomial is
 * XORed with the value in the data stream until each bit in the data stream
 * has been XORed.  The general details of the CRC algorithm are publicly
 * available.
 *
 * The bfn_hash_algorithm_t holds certain parameters that impact the crc
 * calculation:
 *
 *     poly - The polynomial to XOR with the data stream.  Currently assumed
 *         that the upper bit is always 1, and is not stored directly in the
 *         polynomial.  This is done to hold all possible polynomials for a
 *         64 bit crc.
 *     init - The initial value of the crc.  THIS IS XORed with the first
 *         portion of the data stream. (Note: not appended to the msb of the
 *         data stream)
 *     reverse - This indicates two values.  First, the byte ordering, remains
 *         the same, but each byte of the crc is msb to lsb, rather than the
 *         standard lsb to msb.  Secondly, the output of the crc is
 *         bit-reversed as well.
 *     final_xor - This is a value that is XORed at the end of the calculation.
 *         This doesn't apply to the hash_matrix, but the seed, as isn't
 *         necessary for this function
 *
 * In order to fully convert this to a Galois Matrix, a couple extra steps
 * are required.  If the crc has an init value, then the all 0 input to the
 * matrix will have a non all 0 result before the final xor.  However, in
 * a matrix multiplication, no matter what, the all zero input will be an all
 * zero output.
 *
 * Thus in order to capture the all 0 input, the seed register per hash
 * function must represent the result of the all 0 input of the crc function,
 * as it must capture this init value.  This seed value must then be xored
 * out of each matrix row calculation
 */
bool determine_crc_gfm(const bfn_hash_algorithm_t *alg,
                       uint64_t *init_data,
                       uint32_t bitvec_sz,
                       uint32_t input_bit,
                       bool use_input_bit,
                       uint32_t p4_hash_output_bit,
                       uint32_t total_input_bits) {
  // The input stream is 0 extended to the byte.  Always start with highest
  // bit, as constants could be included in the calculation
  uint32_t total_msb = alg->hash_bit_width - 1;
  total_msb += ((total_input_bits + 7) / 8) * 8;

  uint32_t val_bit = input_bit;
  // Reverse the input bytes
  if (alg->reverse) {
    val_bit ^= 7;
  }

  // This is the data stream, and is saved as a bit vector
  uint64_t val[bitvec_sz];
  memset(val, 0, sizeof(val));
  for (uint32_t i = 0; i < bitvec_sz; i++) val[i] = init_data[i];

  if (use_input_bit) {
    int val_bit_pos = val_bit + alg->hash_bit_width;
    set_into_bitvec(val, bitvec_sz, 1ULL, val_bit_pos, 1);
  }

  ///> XOR the initial value into the data stream
  if (alg->init != 0ULL) {
    int init_lsb = total_msb - (alg->hash_bit_width - 1);
    uint64_t current_value =
        get_from_bitvec(val, bitvec_sz, init_lsb, alg->hash_bit_width);
    current_value ^= alg->init;
    set_into_bitvec(
        val, bitvec_sz, current_value, init_lsb, alg->hash_bit_width);
  }

  ///> Calculate crc at each bit position
  for (int i = total_msb - alg->hash_bit_width; i >= 0; i--) {
    int upper_bit_position = (i + alg->hash_bit_width);
    uint64_t upper_value =
        get_from_bitvec(val, bitvec_sz, upper_bit_position, 1);
    upper_value &= 1ULL;
    if (upper_value == 1ULL) {
      set_into_bitvec(val, bitvec_sz, 0ULL, upper_bit_position, 1);
      uint64_t current_value =
          get_from_bitvec(val, bitvec_sz, i, alg->hash_bit_width);
      current_value ^= alg->poly;
      set_into_bitvec(val, bitvec_sz, current_value, i, alg->hash_bit_width);
    }
  }

  ///> Xor the final xor
  uint64_t hash_calc =
      get_from_bitvec(val, bitvec_sz, 0, alg->hash_bit_width) ^ alg->final_xor;

  ///> Reverse the output bits
  uint64_t mask = (1ULL << p4_hash_output_bit);
  if (alg->reverse) {
    mask = (1ULL << ((alg->hash_bit_width - 1) - p4_hash_output_bit));
  }

  return (mask & hash_calc) != 0ULL;
}

bool get_matrix_bit(
    hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN],
    uint32_t ixbar_input_bit,
    uint32_t galois_bit) {
  uint32_t matrix_index0 = ixbar_input_bit / 64;
  uint32_t matrix_index1 = galois_bit;

  uint32_t bit_offset = ixbar_input_bit % 64;
  return (hash_matrix[matrix_index0][matrix_index1].column_value &
          (1ULL << bit_offset)) != 0ULL;
}

void initialize_matrix_bit(
    hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN],
    uint32_t ixbar_input_bit,
    uint32_t galois_bit,
    bool set_bit) {
  uint32_t matrix_index0 = ixbar_input_bit / 64;
  uint32_t matrix_index1 = galois_bit;

  uint32_t bit_offset = ixbar_input_bit % 64;
  uint32_t byte_offset = bit_offset / 8;
  if (set_bit)
    hash_matrix[matrix_index0][matrix_index1].column_value |=
        (1ULL << bit_offset);
  hash_matrix[matrix_index0][matrix_index1].byte_used |= (1ULL << byte_offset);
}

/**
 * The purpose of this function is to determine the bit for hash matrix
 * position input_bit x output_bit is set to 1 or 0.  The parameters
 * for the function are the following:
 *
 * Note that the hash_matrix[ixbar_input_bit][galois_bit] is always the
 * bit which is modified. So it doesn't depend upon the logical input_bit
 * or logical p4_hash_output_bit. The content of this bit, however,
 * depends upon the logical input/output.
 *
 *     ixbar_input_bit - The bit on the 1024 bit input xbar array.
 *     input_bit - the logical input bit on the total list of field bits.
 *     total_input_bits - the size of the input stream for the hash
 *         calculation.  This is required for crc algorithms.
 *     p4_hash_output_bit - The logical hash bit output
 *     galois_bit - the output bit of the 52 bits hash calculation
 *     total_output_bits - the total size of the hash calculation. Required
 *         for msb/extensions of hash calculations.
 */
void determine_matrix_bit(
    hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN],
    const bfn_hash_algorithm_t *alg,
    const ixbar_input_t *inputs,
    uint32_t inputs_sz,
    uint32_t ixbar_input_bit,
    uint32_t input_bit,
    uint32_t total_input_bits,
    uint32_t p4_hash_output_bit,
    uint32_t galois_bit) {
  bool set_bit = false;

  uint32_t virtual_p4_hash_output_bit = p4_hash_output_bit;
  if (alg->extend) {
    virtual_p4_hash_output_bit =
        virtual_p4_hash_output_bit % alg->hash_bit_width;
  }

  ///> Initialize the input bitvec with the constant inputs
  if (alg->hash_alg == IDENTITY_DYN) {
    set_bit = determine_identity_gfm(input_bit, virtual_p4_hash_output_bit);
  } else if (alg->hash_alg == RANDOM_DYN) {
    set_bit = determine_random_gfm();
  } else if (alg->hash_alg == XOR_DYN) {
    set_bit = determine_xor_gfm(input_bit,
                                virtual_p4_hash_output_bit,
                                alg->hash_bit_width,
                                total_input_bits);
  } else if (alg->hash_alg == CRC_DYN) {
    uint32_t bitvec_sz = compute_bitvec_sz(alg, total_input_bits);
    uint64_t val[bitvec_sz];
    memset(val, 0, sizeof(val));
    initialize_input_data_stream(alg, val, bitvec_sz, inputs, inputs_sz);
    // See comments above the dete
    set_bit = determine_crc_gfm(alg,
                                val,
                                bitvec_sz,
                                input_bit,
                                true,
                                virtual_p4_hash_output_bit,
                                total_input_bits) ^
              determine_crc_gfm(alg,
                                val,
                                bitvec_sz,
                                0, /* input_bit */
                                false,
                                virtual_p4_hash_output_bit,
                                total_input_bits);
  }

  initialize_matrix_bit(hash_matrix, ixbar_input_bit, galois_bit, set_bit);
}

/**
 * This function is used to determine the seed.  For CRC functions, this
 * coordinates to the all 0 input result as described in the determine_crc_gfm
 * comments.  For random functions, this will provide a random number as
 * the seed.
 */
void determine_seed_bit(hash_seed_t *hash_seed,
                        uint64_t *init_data, /* bit vec */
                        uint32_t init_data_sz,
                        const bfn_hash_algorithm_t *alg,
                        uint32_t p4_hash_output_bit,
                        uint32_t total_input_bits,
                        uint32_t galois_bit) {
  bool set_bit = false;
  if (alg->hash_alg == RANDOM_DYN) {
    set_bit = random() & 1;
  } else if (alg->hash_alg == CRC_DYN && total_input_bits != 0) {
    if (alg->extend) {
      p4_hash_output_bit = p4_hash_output_bit % alg->hash_bit_width;
    }
    set_bit = determine_crc_gfm(alg,
                                init_data,
                                init_data_sz,
                                0,
                                false,
                                p4_hash_output_bit,
                                total_input_bits);
  } else if (alg->hash_alg == IDENTITY_DYN) {
    set_bit =
        get_from_bitvec(init_data, init_data_sz, p4_hash_output_bit, 1) == 1ULL;
  } else if (alg->hash_alg == XOR_DYN) {
    if (p4_hash_output_bit < ((uint32_t)alg->hash_bit_width)) {
      uint64_t acc = 0;
      int32_t i = p4_hash_output_bit - alg->hash_bit_width +
                  total_input_bits % alg->hash_bit_width;
      if (i < 0) i += alg->hash_bit_width;
      for (; i < ((int32_t)total_input_bits); i += alg->hash_bit_width) {
        acc ^= get_from_bitvec(init_data, init_data_sz, i, 1);
      }
      set_bit = acc == 1ULL;
    } else {
      set_bit = false;
    }
  }
  hash_seed->hash_seed_used |= (1ULL << galois_bit);

  if (set_bit) hash_seed->hash_seed_value |= (1ULL << galois_bit);
}

void print_hash_matrix(
    hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN]) {
  printf("Hash matrix ---> \n");
  uint32_t n_nibbles = 16;
  for (uint32_t i = 0; i < PARITY_GROUPS_DYN; i++) {
    for (uint32_t j = 0; j < n_nibbles; j++) {
      for (uint32_t k = 0; k < HASH_MATRIX_WIDTH_DYN; k++) {
        printf("%" PRIx64 " ",
               ((hash_matrix[i][k].column_value >> (j * 4)) & 0xf));
      }
      printf("\n");
    }
  }
}

void print_hash_matrix_used(
    hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN]) {
  printf("Hash matrix Used bytes ---> \n");
  for (uint32_t i = 0; i < PARITY_GROUPS_DYN; i++) {
    for (uint32_t j = 0; j < HASH_MATRIX_WIDTH_DYN; j++) {
      printf("%x ", hash_matrix[i][j].byte_used);
    }
    printf("\n");
  }
}

static void rotate_seed(hash_calc_rotate_info_t *rot_info,
                        hash_seed_t *hash_seed) {
  uint32_t bit_posn = 0, idx = 0;
  bool hash_seed_value[HASH_SEED_LENGTH] = {0};
  bool hash_seed_used[HASH_SEED_LENGTH] = {0};
  bool rotate_temp_value[HASH_SEED_LENGTH] = {0};
  bool rotate_temp_used[HASH_SEED_LENGTH] = {0};
  uint64_t rotate = rot_info->rotate;
  uint32_t num_tot_hash_bits = rot_info->num_hash_bits;
  /* Dissect into an array */
  while (bit_posn < num_tot_hash_bits) {
    idx = rot_info->p4_hash_output_bit_posn[bit_posn];
    hash_seed_value[idx] = hash_seed->hash_seed_value & (1ULL << idx);
    hash_seed_used[idx] = hash_seed->hash_seed_used & (1ULL << idx);
    bit_posn++;
  }

  /* Safeguard the rotated bits. */
  bit_posn = 0;
  while (bit_posn < rotate) {
    idx = rot_info->p4_hash_output_bit_posn[(num_tot_hash_bits - 1) -
                                            (rotate - 1) + bit_posn];
    rotate_temp_value[bit_posn] = hash_seed_value[idx];
    rotate_temp_used[bit_posn] = hash_seed_used[idx];
    bit_posn++;
  }

  /* Right shift as per rotation. */
  for (int rot_idx = num_tot_hash_bits - (rotate + 1); rot_idx >= 0;
       rot_idx--) {
    idx = rot_info->p4_hash_output_bit_posn[rot_idx];
    hash_seed_value[(idx + rotate) % HASH_SEED_LENGTH] = hash_seed_value[idx];
    hash_seed_used[(idx + rotate) % HASH_SEED_LENGTH] = hash_seed_used[idx];
  }

  /* Get rotated bits to position. */
  for (bit_posn = 0; bit_posn < rotate; bit_posn++) {
    idx = rot_info->p4_hash_output_bit_posn[bit_posn];
    hash_seed_value[idx] = rotate_temp_value[bit_posn];
    hash_seed_used[idx] = rotate_temp_used[bit_posn];
  }
  bit_posn = 0;
  /* Reform the seed. */
  while (bit_posn < num_tot_hash_bits) {
    idx = rot_info->p4_hash_output_bit_posn[bit_posn];
    /* Reset the original bit posn value. */
    hash_seed->hash_seed_value &= ~(1ULL << idx);
    hash_seed->hash_seed_used &= ~(1ULL << idx);
    if (hash_seed_value[idx]) {
      hash_seed->hash_seed_value |= (1ULL << idx);
    }
    if (hash_seed_used[idx]) {
      hash_seed->hash_seed_used |= (1ULL << idx);
    }
    bit_posn++;
  }
}

static void rotate_matrix(
    hash_calc_rotate_info_t *rot_info,
    hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN]) {
  uint32_t p4_hash_bit_idx = 0, idx = 0;
  hash_column_t temp_col[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN];
  uint64_t rotate = rot_info->rotate;
  uint32_t num_tot_hash_bits = rot_info->num_hash_bits;
  /* Safeguard the rotated coloumn. */
  for (uint32_t i = 0; i < rotate; i++) {
    for (uint32_t k = 0; k < PARITY_GROUPS_DYN; k++) {
      p4_hash_bit_idx =
          rot_info->p4_hash_output_bit_posn[(num_tot_hash_bits - 1) -
                                            (rotate - 1) + i];
      idx = rot_info->gfm_bit_posn[p4_hash_bit_idx];
      temp_col[k][i].column_value = hash_matrix[k][idx].column_value;
      temp_col[k][i].byte_used = hash_matrix[k][idx].byte_used;
    }
  }

  /* Right shift as per rotation. */
  for (int i = num_tot_hash_bits - (rotate + 1); i >= 0; i--) {
    for (uint32_t k = 0; k < PARITY_GROUPS_DYN; k++) {
      p4_hash_bit_idx = rot_info->p4_hash_output_bit_posn[i];
      idx = rot_info->gfm_bit_posn[p4_hash_bit_idx];
      hash_matrix[k][(idx + rotate) % num_tot_hash_bits].column_value =
          hash_matrix[k][idx].column_value;
      hash_matrix[k][(idx + rotate) % num_tot_hash_bits].byte_used =
          hash_matrix[k][idx].byte_used;
    }
  }

  /* Get rotated bits to position. */
  for (uint32_t i = 0; i < rotate; i++) {
    for (uint32_t k = 0; k < PARITY_GROUPS_DYN; k++) {
      p4_hash_bit_idx = rot_info->p4_hash_output_bit_posn[i];
      idx = rot_info->gfm_bit_posn[p4_hash_bit_idx];
      hash_matrix[k][idx].column_value = temp_col[k][i].column_value;
      hash_matrix[k][idx].byte_used = temp_col[k][i].byte_used;
    }
  }
}

/**
 * Updates the hash matrix for symmetric fields.
 * Every field taking part in symmetric hashing has a sym_group and
 * a sib_sym_group. For example if (src_ip, dst_ip) are symmetrically
 * hashed, src_ip bits can have group as 1 and sib_sym_group as 2.
 * The reverse for dst_ip. Use these groups to figure out which field
 * inputs need to be XOR'ed.
 * The following property is used if (f2,f3) are symmetric and
 * (f4,f5) are symmetric.
 *
 * symmetric(f1,f2,f3,f4,f5) = crc(f1,f2,f3,f4,f5) ^ crc(f1,f3,f2,f5,f4)
 * == crc(f1, f2^f3, f3^f2, f4^f5, f5^f4)
 *
 */
void update_symmetric_bits(
    const ixbar_init_t *ixbar_init,
    const ixbar_input_t *inputs,
    uint32_t inputs_sz,
    hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN]) {
  uint32_t n_groups = 0;
  uint32_t *groups = NULL;
  uint32_t *sibling_groups = NULL;
  bool symmetric_present = false;
  hash_column_t hash_matrix_old[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN];

  // Check the input first. If there are no symmetric fields present,
  // return
  for (uint32_t i = 0; i < inputs_sz; i++) {
    const ixbar_input_t *input = inputs + i;
    if (input->type == tPHV && !input->u.valid) continue;
    if (input->type == tCONST) continue;
    if (input->symmetric_info.is_symmetric == false) continue;
    symmetric_present = true;
  }
  if (!symmetric_present) return;

  memcpy(hash_matrix_old,
         hash_matrix,
         sizeof(hash_column_t) * PARITY_GROUPS_DYN * HASH_MATRIX_WIDTH_DYN);

  groups = calloc_macro(inputs_sz, sizeof(uint32_t));
  ALLOC_CHECK(groups, "failed to allocate groups");

  sibling_groups = calloc_macro(inputs_sz, sizeof(uint32_t));
  ALLOC_CHECK(sibling_groups, "failed to allocate sibling_groups");

  // preprocess the input and record all the groups and sibling groups
  for (uint32_t i = 0; i < inputs_sz; i++) {
    const ixbar_input_t *input = inputs + i;
    if (input->type == tPHV && !input->u.valid) continue;
    if (input->type == tCONST) continue;
    if (input->symmetric_info.is_symmetric == false) continue;

    groups[n_groups] = input->symmetric_info.sym_group;
    sibling_groups[n_groups] = input->symmetric_info.sib_sym_group;
    n_groups++;
  }

  for (uint32_t g = 0; g < n_groups; g++) {
    uint32_t cur_sym_count = 0;
    for (uint32_t i = 0; i < inputs_sz; i++) {
      const ixbar_input_t *input = inputs + i;
      if (input->type == tPHV && !input->u.valid) continue;
      if (input->type == tCONST) continue;
      if (input->symmetric_info.is_symmetric == false) continue;
      // Process each group at a time
      if (input->symmetric_info.sym_group != groups[g]) continue;

      uint32_t sib_sym_count = 0;
      const ixbar_input_t *sib_input = NULL;
      bool sib_found = false;
      // Find the corresponding sibling bit
      for (uint32_t a = 0; a < inputs_sz; a++) {
        sib_input = inputs + a;
        // Check if the prospective sibling's sym_group is the same as
        // the sibling group we are trying to find
        if (sib_input->symmetric_info.sym_group == sibling_groups[g]) {
          if (cur_sym_count == sib_sym_count) {
            sib_found = true;
            break;
          }
          sib_sym_count++;
          if (sib_sym_count > cur_sym_count) break;
        }
      }
      // If sibling wasn't found, just continue
      if (!sib_found) continue;
      // Sibling was found. A few sanity checks like comparing bit_size
      if (input->bit_size != sib_input->bit_size) continue;

      cur_sym_count++;

      // modify hash_matrix for input with xor of sib_input

      for (uint32_t j = 0; j < input->bit_size; j++) {
        uint32_t ixbar_input_bit = input->ixbar_bit_position + j;
        uint32_t sib_ixbar_input_bit = sib_input->ixbar_bit_position + j;

        for (uint32_t k = 0; k < ixbar_init->outputs_sz; k++) {
          hash_matrix_output_t *output = ixbar_init->hash_matrix_outputs + k;
          for (uint32_t l = 0; l < output->bit_size; l++) {
            uint32_t galois_bit = output->gfm_start_bit + l;
            bool set_bit =
                get_matrix_bit(
                    hash_matrix_old, sib_ixbar_input_bit, galois_bit) ^
                get_matrix_bit(hash_matrix_old, ixbar_input_bit, galois_bit);
            initialize_matrix_bit(
                hash_matrix, ixbar_input_bit, galois_bit, set_bit);
          }
        }
      }
    }
  }
  if (groups) free_macro(groups);
  if (sibling_groups) free_macro(sibling_groups);
}

void determine_hash_matrix(
    const ixbar_init_t *ixbar_init,
    const ixbar_input_t *inputs,
    uint32_t inputs_sz,
    const bfn_hash_algorithm_t *alg,
    hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN]) {
  uint32_t total_input_bits = 0;
  for (uint32_t i = 0; i < inputs_sz; i++) {
    const ixbar_input_t *input = inputs + i;
    total_input_bits += input->bit_size;
  }

  ///> Initialize the GFM matrix with the corresponding inputs
  int input_bit = 0;
  for (uint32_t i = 0; i < inputs_sz; i++) {
    const ixbar_input_t *input = inputs + i;
    ///> Potential holes in inputs during partial crc calculations during the
    ///  compiler require valid inputs
    if (input->type == tPHV && !input->u.valid) {
      input_bit += input->bit_size;
      continue;
    } else if (input->type == tCONST) {
      input_bit += input->bit_size;
      continue;
    }

    ///> Iterate through all inputs and all outputs bit by bit to calculate the
    ///  bit positions
    for (uint32_t j = 0; j < input->bit_size; j++) {
      uint32_t ixbar_input_bit = input->ixbar_bit_position + j;
      for (uint32_t k = 0; k < ixbar_init->outputs_sz; k++) {
        const hash_matrix_output_t *output =
            ixbar_init->hash_matrix_outputs + k;
        for (uint32_t l = 0; l < output->bit_size; l++) {
          uint32_t p4_hash_output_bit = output->p4_hash_output_bit + l;
          uint32_t galois_bit = output->gfm_start_bit + l;
          determine_matrix_bit(hash_matrix,
                               alg,
                               inputs,
                               inputs_sz,
                               ixbar_input_bit,
                               input_bit,
                               total_input_bits,
                               p4_hash_output_bit,
                               galois_bit);
        }
      }
      input_bit++;
    }
  }

  ///> Mark all fields not included in the inputs as used and marked as 0
  for (uint32_t i = 0; i < ixbar_init->inputs_sz; i++) {
    ixbar_input_t *input = ixbar_init->ixbar_inputs + i;
    if (input->type == tPHV && !input->u.valid) {
      continue;
    }
    if (input->type == tCONST) continue;
    for (uint32_t j = 0; j < input->bit_size; j++) {
      uint32_t ixbar_input_bit = input->ixbar_bit_position + j;
      for (uint32_t k = 0; k < ixbar_init->outputs_sz; k++) {
        hash_matrix_output_t *output = ixbar_init->hash_matrix_outputs + k;
        for (uint32_t l = 0; l < output->bit_size; l++) {
          uint32_t galois_bit = output->gfm_start_bit + l;
          initialize_matrix_bit(
              hash_matrix, ixbar_input_bit, galois_bit, false);
        }
      }
    }
  }
}

void determine_seed(const hash_matrix_output_t *hash_matrix_outputs,
                    uint32_t outputs_sz,
                    const ixbar_input_t *inputs,
                    uint32_t inputs_sz,
                    uint32_t total_input_bits,
                    const bfn_hash_algorithm_t *alg,
                    hash_seed_t *hash_seed) {
  ///> Initialize the input bitvec with the constant inputs
  uint32_t bitvec_sz = compute_bitvec_sz(alg, total_input_bits);
  // This is the data stream, and is saved as a bit vector
  uint64_t val[bitvec_sz];
  memset(val, 0, sizeof(val));
  initialize_input_data_stream(alg, val, bitvec_sz, inputs, inputs_sz);

  for (uint32_t i = 0; i < outputs_sz; i++) {
    const hash_matrix_output_t *output = hash_matrix_outputs + i;
    for (uint32_t j = 0; j < output->bit_size; j++) {
      uint32_t p4_hash_output_bit = output->p4_hash_output_bit + j;
      uint32_t galois_bit = output->gfm_start_bit + j;
      determine_seed_bit(hash_seed,
                         val,
                         bitvec_sz,
                         alg,
                         p4_hash_output_bit,
                         total_input_bits,
                         galois_bit);
    }
  }
}

/**
 * This builds the changes to the galois field matrix for this particular hash
 * function.
 */
void determine_tofino_gfm_delta(
    hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN],
    hash_regs_t *regs) {
  int galois_delta_count = 0;
  ///> Determine the number of registers that require a change
  for (int i = 0; i < PARITY_GROUPS_DYN; i++) {
    for (int j = 0; j < HASH_MATRIX_WIDTH_DYN; j++) {
      for (int k = 0; k < 8; k += 2) {
        if (((1 << k) & hash_matrix[i][j].byte_used) != 0 ||
            ((1 << (k + 1)) & hash_matrix[i][j].byte_used) != 0) {
          galois_delta_count++;
        }
      }
    }
  }

  regs->galois_field_matrix_regs =
      calloc_macro(galois_delta_count, sizeof(galois_field_matrix_delta_t));
  if (!regs->galois_field_matrix_regs) {
    assert_macro(0);
    return;
  }

  regs->gfm_sz = galois_delta_count;

  ///> Build the array of registers that require the change.
  uint32_t gfm_index = 0;
  for (int i = 0; i < PARITY_GROUPS_DYN; i++) {
    for (int j = 0; j < HASH_MATRIX_WIDTH_DYN; j++) {
      for (int k = 0; k < 8; k += 2) {
        int byte0_shift = k * 8;
        int byte1_shift = (k + 1) * 8;

        if (((1 << k) & hash_matrix[i][j].byte_used) == 0 &&
            ((1 << (k + 1)) & hash_matrix[i][j].byte_used) == 0)
          continue;

        // NOTE: Must be freed after use outside of this
        galois_field_matrix_delta_t *gfm_delta =
            regs->galois_field_matrix_regs + gfm_index;
        gfm_delta->byte_pair_index = i * BYTE_PAIRS_PER_PARITY_GROUP + k / 2;
        gfm_delta->hash_bit = j;
        gfm_delta->byte0 =
            (hash_matrix[i][j].column_value >> byte0_shift) & 0xffUL;
        gfm_delta->byte1 =
            (hash_matrix[i][j].column_value >> byte1_shift) & 0xffUL;
        gfm_delta->valid0 = 0;
        gfm_delta->valid1 = 0;
        gfm_index++;
      }
    }
  }
}

void determine_seed_delta(const hash_seed_t *hash_seed,
                          hash_regs_t *regs,
                          uint32_t parity_group) {
  int seed_delta_count = 0;

  ///> Determine the number of registers that require a change
  for (int i = 0; i < HASH_MATRIX_WIDTH_DYN; i++) {
    if (((1ULL << i) & hash_seed->hash_seed_used) != 0ULL) seed_delta_count++;
  }

  // NOTE: Must be freed after use outside of this
  regs->hash_seed_regs =
      calloc_macro(seed_delta_count, sizeof(hash_seed_delta_t));
  if (!regs->hash_seed_regs) {
    assert_macro(0);
    return;
  }

  regs->hs_sz = seed_delta_count;

  ///> Build the array of registers that require the change
  uint32_t seed_index = 0;
  for (int i = 0; i < HASH_MATRIX_WIDTH_DYN; i++) {
    uint64_t mask = (1ULL << i);
    if ((mask & hash_seed->hash_seed_used) == 0ULL) continue;
    uint64_t seed_value = (mask & hash_seed->hash_seed_value) >> i;
    hash_seed_delta_t *hs_delta = regs->hash_seed_regs + seed_index;

    hs_delta->hash_bit = i;
    hs_delta->hash_seed_and_value = 0xff & ~(1 << parity_group);
    hs_delta->hash_seed_or_value = (seed_value << parity_group);
    seed_index++;
  }
}

void determine_tofino_regs(const ixbar_init_t *ixbar_init,
                           const ixbar_input_t *inputs,
                           uint32_t inputs_sz,
                           const bfn_hash_algorithm_t *alg,
                           hash_calc_rotate_info_t *rot_info,
                           hash_regs_t *regs) {
  hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN];
  memset(hash_matrix,
         0,
         sizeof(hash_column_t) * PARITY_GROUPS_DYN * HASH_MATRIX_WIDTH_DYN);

  hash_seed_t hash_seed;
  memset(&hash_seed, 0, sizeof(hash_seed_t));

  uint32_t total_input_bits = 0;
  for (uint32_t i = 0; i < inputs_sz; i++) {
    const ixbar_input_t *input = inputs + i;
    total_input_bits += input->bit_size;
  }
  determine_hash_matrix(ixbar_init, inputs, inputs_sz, alg, hash_matrix);
  update_symmetric_bits(ixbar_init, inputs, inputs_sz, hash_matrix);
  if (rot_info->rotate) {
    rotate_matrix(rot_info, hash_matrix);
  }
#ifdef HASH_DEBUG
  print_hash_matrix(hash_matrix);
  print_hash_matrix_used(hash_matrix);
#endif
  determine_seed(ixbar_init->hash_matrix_outputs,
                 ixbar_init->outputs_sz,
                 inputs,
                 inputs_sz,
                 total_input_bits,
                 alg,
                 &hash_seed);
  if (rot_info->rotate) {
    rotate_seed(rot_info, &hash_seed);
  }

  determine_tofino_gfm_delta(hash_matrix, regs);
  determine_seed_delta(&hash_seed, regs, ixbar_init->parity_group);
}
