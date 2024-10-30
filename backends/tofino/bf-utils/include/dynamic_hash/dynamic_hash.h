#ifndef BACKENDS_TOFINO_BF_UTILS_INCLUDE_DYNAMIC_HASH_DYNAMIC_HASH_H_
#define BACKENDS_TOFINO_BF_UTILS_INCLUDE_DYNAMIC_HASH_DYNAMIC_HASH_H_

#include <stdbool.h>
#include <stdint.h>

#include "bfn_hash_algorithm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PARITY_GROUPS_DYN 16
#define HASH_MATRIX_WIDTH_DYN 52
#define HASH_SEED_LENGTH 64
/* 128 crossbar bytes divided by 16 parity groups divided by 2 per byte pair */
#define BYTE_PAIRS_PER_PARITY_GROUP 4

enum ixbar_input_type { tCONST, tPHV };

/**
 * @brief Contains Symmetric hashing info for a hash input.
 */
typedef struct hash_symmetric_info_ {
    /* Whether this bit-range is symmetric */
    bool is_symmetric;
    /* The symmetric group for this hash input */
    uint32_t sym_group;
    /* The symmetric group which this hash input
     * needs to be symmetrically hashed with */
    uint32_t sib_sym_group;
} hash_symmetric_info_t;

/**
 * Represents a single bit range of inputs from phv.
 *     ixbar_bit_position - the start_bit on the 1024 bit IXBar
 *     bit_size - the bits in the input
 *     symmetric_info - contains info regarding symmetric hashing. If 2 fields
 *        f1 and f2 need to be symmetric, then their respective "sib_sym_group"
 *        need to point to the other's group. Their respective order in
 *        the input ixbar vec in honored. So the first input encountered having
 *        sibling group 2, self 1, we search for the first input having self
 *        group 2.
 *     valid - whether a hash matrix should be calculated for this input,
 *             only used when type is tPHV
 *     constant - constant value in a hash field list. current max size is 64
 *             bit. This is a software constraint. User can write multiple
 *             constant values in a field list if a larger constant is needed.
 */
typedef struct ixbar_input_ {
    enum ixbar_input_type type;
    uint32_t ixbar_bit_position;
    uint32_t bit_size;
    hash_symmetric_info_t symmetric_info;
    union {
        bool valid;
        uint64_t constant;
    } u;
} ixbar_input_t;

/**
 * Represents a single bit range of hash output
 *     p4_hash_output_bit - start bit of the range when coordinated to the P4
 *         hash function
 *     gfm_start_bit - The first bit in the range of the 52 bit galois
 *          matrix
 *     bit_size - the number of bits in the range
 */
typedef struct hash_calc_output_ {
    uint32_t p4_hash_output_bit;
    uint32_t gfm_start_bit;
    uint32_t bit_size;
} hash_matrix_output_t;

/**
 * Represents the hash bit ranges and rotation of hash output
 *     rotate - bits by which the hash has to be rotated
 *     num_hash_bits - number of hash bits in the hash function
 *     gfm_bit_posn - pointer to array of bits in the range of the 52 bit GFM.
 *        Size is num_hash_bits. Contains the columns corresponding to the
 *        p4_hash_output_bit_posn value at the same index.
 *     p4_hash_output_bit_posn - pointer to array of the arrangement of the
 *        p4 hash output bits. Size is num_hash_bits. Contains the bit position
 *        of the p4 hash ouput bits indexed by the num_hash_bits.
 */
typedef struct hash_calc_rotate_info {
    uint64_t rotate;
    uint32_t num_hash_bits;
    uint32_t *gfm_bit_posn;
    uint32_t *p4_hash_output_bit_posn;
} hash_calc_rotate_info_t;

/**
 * Storage of all inputs and outputs for this calculation.  The parity group
 * corresponds to which of the 8 hash outputs this hash function is output to.
 */
typedef struct ixbar_init_ {
    ixbar_input_t *ixbar_inputs;
    uint32_t inputs_sz;

    hash_matrix_output_t *hash_matrix_outputs;
    uint32_t outputs_sz;

    uint32_t parity_group;
} ixbar_init_t;

/**
 * Representation of an inidvidual 64 bit hash matrix column.
 *     column_value - bit value of the column
 *     byte_used - bit mask of the bytes used per column
 */
typedef struct hash_column_ {
    uint64_t column_value;
    uint8_t byte_used;
} hash_column_t;

/**
 * Representation of a 52 bit seed per parity group
 *     hash_seed_value - bit value of the hash_seed
 *     hash_seed_used - bit mask of the seed bits used by this calculation
 */
typedef struct hash_seed_ {
    uint64_t hash_seed_value;
    uint64_t hash_seed_used;
} hash_seed_t;

/** The changes to make to an individual galois matrix register. */
typedef struct galois_field_matrix_delta_ {
    uint32_t byte_pair_index;
    uint32_t hash_bit;

    unsigned byte0;
    unsigned valid0;
    unsigned byte1;
    unsigned valid1;
} galois_field_matrix_delta_t;

/** The changes to make to an individual hash seed register */
typedef struct hash_seed_delta_ {
    uint32_t hash_bit;
    unsigned hash_seed_and_value;
    unsigned hash_seed_or_value;
} hash_seed_delta_t;

/** The changes required to make to the hash matrix registers */
typedef struct hash_regs_ {
    // This field must be freed after use
    galois_field_matrix_delta_t *galois_field_matrix_regs;
    uint32_t gfm_sz;

    // This field must be freed after use
    hash_seed_delta_t *hash_seed_regs;
    uint32_t hs_sz;
} hash_regs_t;

/**
 * This function given a list of inputs, outputs, and algorithm, will calculate
 * the galois matrix.  The parameters are the following:
 *
 *    ixbar_init - The list of all inputs and all outputs for a hash matrix
 *        calculation.  This would be a single field list with all optional
 *        fields turned on
 *    inputs - The list of all inputs to be included in this hash matrix
 *        calculation.  All optional fields that are not to be included in
 *        this field list would be left out
 *    inputs_sz - The size of the inputs array
 *    alg - The hash algorithm used to calculate the hash matrix
 *    hash_matrix - The 1024 x 52 b array representing the hash matrix in
 *        Barefoot hardware
 */
void determine_hash_matrix(const ixbar_init_t *ixbar_init, const ixbar_input_t *inputs,
                           uint32_t inputs_sz, const bfn_hash_algorithm_t *alg,
                           hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN]);
/**
 * This function calculates the seed algorithm for a hash matrix.  The
 * parameters are the following:
 *
 *     hash_matrix_outputs - an array of of all outputs of the hash matrix.
 *         This is to coordinate the final_xor value of the hash algorithm
 *         with the allocation in the galois matrix
 *     outputs_sz - the size of the hash_matrix_outputs array
 *     inputs - an array of all of the inputs to the algorithm, which may have
 *         constant values
 *     inputs_sz - the size of the inputs array
 *     total_input_bits - the number of input bits
 *     alg - the algorithm used to calculate the final_xor
 *     hash_seed - The calculated seed for this algorithm
 */
void determine_seed(const hash_matrix_output_t *hash_matrix_outputs, uint32_t outputs_sz,
                    const ixbar_input_t *inputs, uint32_t inputs_sz, uint32_t total_input_bits,
                    const bfn_hash_algorithm_t *alg, hash_seed_t *hash_seed);

/**
 * This function calculates the registers to be updated in order to implement
 * the hash function. The parameters are the following:
 *
 *     ixbar_init - The list of all inputs and all outputs for a hash matrix
 *         calculation.  This would be a single field list with all optional
 *         fields turned on
 *     inputs - The list of all inputs to be included in this hash matrix
 *         calculation.  All optional fields that are not to be included in
 *         this field list would be left out
 *     inputs_sz - The size of the inputs array
 *     alg - The hash algorithm used to calculate the hash matrix
 *     hash_regs - The register deltas needed to update in order to set the
 *         hash_matrix.  This has multiple pointers that have to be freed.
 */
void determine_tofino_regs(const ixbar_init_t *ixbar_init, const ixbar_input_t *inputs,
                           uint32_t input_sz, const bfn_hash_algorithm_t *alg,
                           hash_calc_rotate_info_t *rot_info, hash_regs_t *hash_regs);

#ifdef __cplusplus
}
#endif

#endif /* BACKENDS_TOFINO_BF_UTILS_INCLUDE_DYNAMIC_HASH_DYNAMIC_HASH_H_ */
