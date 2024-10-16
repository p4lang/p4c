#ifndef _BF_UTILS_ALGORITHM_H_
#define _BF_UTILS_ALGORITHM_H_

#ifndef __KERNEL__
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define BF_UTILS_ALGO_NAME_LEN 30

typedef enum {
  CRC_8,
  CRC_8_DARC,
  CRC_8_I_CODE,
  CRC_8_ITU,
  CRC_8_MAXIM,
  CRC_8_ROHC,
  CRC_8_WCDMA,
  CRC_16,
  CRC_16_BYPASS,
  CRC_16_DDS_110,
  CRC_16_DECT,
  CRC_16_DECT_R,
  CRC_16_DECT_X,
  CRC_16_DNP,
  CRC_16_EN_13757,
  CRC_16_GENIBUS,
  CRC_16_MAXIM,
  CRC_16_MCRF4XX,
  CRC_16_RIELLO,
  CRC_16_T10_DIF,
  CRC_16_TELEDISK,
  CRC_16_USB,
  X_25,
  XMODEM,
  MODBUS,
  KERMIT,
  CRC_CCITT_FALSE,
  CRC_AUG_CCITT,
  CRC_32,
  CRC_32_BZIP2,
  CRC_32C,
  CRC_32D,
  CRC_32_MPEG,
  POSIX,
  CRC_32Q,
  JAMCRC,
  XFER,
  CRC_64,
  CRC_64_GO_ISO,
  CRC_64_WE,
  CRC_64_JONES,
  CRC_INVALID
} bfn_crc_alg_t;

typedef enum {
  IDENTITY_DYN,
  CRC_DYN,
  RANDOM_DYN,
  XOR_DYN,
  INVALID_DYN
} bfn_hash_alg_type_t;

static inline void crc_alg_type_to_str(bfn_crc_alg_t crc_type, char *crc_name) {
  if (!crc_name) {
    return;
  }
  switch (crc_type) {
    case CRC_8:
      strncpy(crc_name, "CRC_8\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_8_DARC:
      strncpy(crc_name, "CRC_8_DARC\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_8_I_CODE:
      strncpy(crc_name, "CRC_8_I_CODE\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_8_ITU:
      strncpy(crc_name, "CRC_8_ITU\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_8_MAXIM:
      strncpy(crc_name, "CRC_8_MAXIM\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_8_ROHC:
      strncpy(crc_name, "CRC_8_ROHC\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_8_WCDMA:
      strncpy(crc_name, "CRC_8_WCDMA\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16:
      strncpy(crc_name, "CRC_16\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_BYPASS:
      strncpy(crc_name, "CRC_16_BYPASS\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_DDS_110:
      strncpy(crc_name, "CRC_16_DDS_110\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_DECT:
      strncpy(crc_name, "CRC_16_DECT\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_DECT_R:
      strncpy(crc_name, "CRC_16_DECT_R\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_DECT_X:
      strncpy(crc_name, "CRC_16_DECT_X\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_DNP:
      strncpy(crc_name, "CRC_16_DNP\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_EN_13757:
      strncpy(crc_name, "CRC_16_EN_13757\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_GENIBUS:
      strncpy(crc_name, "CRC_16_GENIBUS\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_MAXIM:
      strncpy(crc_name, "CRC_16_MAXIM\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_MCRF4XX:
      strncpy(crc_name, "CRC_16_MCRF4XX\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_RIELLO:
      strncpy(crc_name, "CRC_16_RIELLO\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_T10_DIF:
      strncpy(crc_name, "CRC_16_T10_DIF\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_TELEDISK:
      strncpy(crc_name, "CRC_16_TELEDISK\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_16_USB:
      strncpy(crc_name, "CRC_16_USB\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case X_25:
      strncpy(crc_name, "X_25\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case XMODEM:
      strncpy(crc_name, "XMODEM\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case MODBUS:
      strncpy(crc_name, "MODBUS\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case KERMIT:
      strncpy(crc_name, "KERMIT\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_CCITT_FALSE:
      strncpy(crc_name, "CRC_CCITT_FALSE\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_AUG_CCITT:
      strncpy(crc_name, "CRC_AUG_CCITT\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_32:
      strncpy(crc_name, "CRC_32\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_32_BZIP2:
      strncpy(crc_name, "CRC_32_BZIP2\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_32C:
      strncpy(crc_name, "CRC_32C\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_32D:
      strncpy(crc_name, "CRC_32D\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_32_MPEG:
      strncpy(crc_name, "CRC_32_MPEG\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case POSIX:
      strncpy(crc_name, "POSIX\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_32Q:
      strncpy(crc_name, "CRC_32Q\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case JAMCRC:
      strncpy(crc_name, "JAMCRC\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case XFER:
      strncpy(crc_name, "XFER\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_64:
      strncpy(crc_name, "CRC_64\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_64_GO_ISO:
      strncpy(crc_name, "CRC_64_GO_ISO\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_64_WE:
      strncpy(crc_name, "CRC_64_WE\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_64_JONES:
      strncpy(crc_name, "CRC_64_JONES\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_INVALID:
    default:
      strncpy(crc_name, "CRC_INVALID\0", BF_UTILS_ALGO_NAME_LEN);
      break;
  }
  return;
}

static inline bfn_crc_alg_t crc_alg_str_to_type(const char *crc_name) {
  if (crc_name == NULL) {
    return CRC_INVALID;
  }
  if (!strcmp(crc_name, "crc_8") || !strcmp(crc_name, "CRC_8")) {
    return CRC_8;
  }
  if (!strcmp(crc_name, "crc_8_darc") || !strcmp(crc_name, "CRC_8_DARC")) {
    return CRC_8_DARC;
  }
  if (!strcmp(crc_name, "crc_8_i_code") || !strcmp(crc_name, "CRC_8_I_CODE")) {
    return CRC_8_I_CODE;
  }
  if (!strcmp(crc_name, "crc_8_itu") || !strcmp(crc_name, "CRC_8_ITU")) {
    return CRC_8_ITU;
  }
  if (!strcmp(crc_name, "crc_8_maxim") || !strcmp(crc_name, "CRC_8_MAXIM")) {
    return CRC_8_MAXIM;
  }
  if (!strcmp(crc_name, "crc_8_rohc") || !strcmp(crc_name, "CRC_8_ROHC")) {
    return CRC_8_ROHC;
  }
  if (!strcmp(crc_name, "crc_8_wcdma") || !strcmp(crc_name, "CRC_8_WCDMA")) {
    return CRC_8_WCDMA;
  }
  if (!strcmp(crc_name, "crc_16") || !strcmp(crc_name, "CRC_16")) {
    return CRC_16;
  }
  if (!strcmp(crc_name, "crc_16_bypass") ||
      !strcmp(crc_name, "CRC_16_BYPASS")) {
    return CRC_16_BYPASS;
  }
  if (!strcmp(crc_name, "crc_16_dds_110") ||
      !strcmp(crc_name, "CRC_16_DDS_110")) {
    return CRC_16_DDS_110;
  }
  if (!strcmp(crc_name, "crc_16_dect") || !strcmp(crc_name, "CRC_16_DECT")) {
    return CRC_16_DECT;
  }
  if (!strcmp(crc_name, "crc_16_dect_r") ||
      !strcmp(crc_name, "CRC_16_DECT_R")) {
    return CRC_16_DECT_R;
  }
  if (!strcmp(crc_name, "crc_16_dect_x") ||
      !strcmp(crc_name, "CRC_16_DECT_X")) {
    return CRC_16_DECT_X;
  }
  if (!strcmp(crc_name, "crc_16_dnp") || !strcmp(crc_name, "CRC_16_DNP")) {
    return CRC_16_DNP;
  }
  if (!strcmp(crc_name, "crc_16_en_13757") ||
      !strcmp(crc_name, "CRC_16_EN_13757")) {
    return CRC_16_EN_13757;
  }
  if (!strcmp(crc_name, "crc_16_genibus") ||
      !strcmp(crc_name, "CRC_16_GENIBUS")) {
    return CRC_16_GENIBUS;
  }
  if (!strcmp(crc_name, "crc_16_maxim") || !strcmp(crc_name, "CRC_16_MAXIM")) {
    return CRC_16_MAXIM;
  }
  if (!strcmp(crc_name, "crc_16_mcrf4xx") ||
      !strcmp(crc_name, "CRC_16_MCRF4XX")) {
    return CRC_16_MCRF4XX;
  }
  if (!strcmp(crc_name, "crc_16_riello") ||
      !strcmp(crc_name, "CRC_16_RIELLO")) {
    return CRC_16_RIELLO;
  }
  if (!strcmp(crc_name, "crc_16_t10_dif") ||
      !strcmp(crc_name, "CRC_16_T10_DIF")) {
    return CRC_16_T10_DIF;
  }
  if (!strcmp(crc_name, "crc_16_teledisk") ||
      !strcmp(crc_name, "CRC_16_TELEDISK")) {
    return CRC_16_TELEDISK;
  }
  if (!strcmp(crc_name, "crc_16_usb") || !strcmp(crc_name, "CRC_16_USB")) {
    return CRC_16_USB;
  }
  if (!strcmp(crc_name, "x_25") || !strcmp(crc_name, "X_25")) {
    return X_25;
  }
  if (!strcmp(crc_name, "xmodem") || !strcmp(crc_name, "XMODEM")) {
    return XMODEM;
  }
  if (!strcmp(crc_name, "modbus") || !strcmp(crc_name, "MODBUS")) {
    return MODBUS;
  }
  if (!strcmp(crc_name, "kermit") || !strcmp(crc_name, "KERMIT")) {
    return KERMIT;
  }
  if (!strcmp(crc_name, "crc_ccitt_false") ||
      !strcmp(crc_name, "CRC_CCITT_FALSE")) {
    return CRC_CCITT_FALSE;
  }
  if (!strcmp(crc_name, "crc_aug_ccitt") ||
      !strcmp(crc_name, "CRC_AUG_CCITT")) {
    return CRC_AUG_CCITT;
  }
  if (!strcmp(crc_name, "crc_32") || !strcmp(crc_name, "CRC_32")) {
    return CRC_32;
  }
  if (!strcmp(crc_name, "crc_32_bzip2") || !strcmp(crc_name, "CRC_32_BZIP2")) {
    return CRC_32_BZIP2;
  }
  if (!strcmp(crc_name, "crc_32c") || !strcmp(crc_name, "CRC_32C")) {
    return CRC_32C;
  }
  if (!strcmp(crc_name, "crc_32d") || !strcmp(crc_name, "CRC_32D")) {
    return CRC_32D;
  }
  if (!strcmp(crc_name, "crc_32_mpeg") || !strcmp(crc_name, "CRC_32_MPEG")) {
    return CRC_32_MPEG;
  }
  if (!strcmp(crc_name, "posix") || !strcmp(crc_name, "POSIX")) {
    return POSIX;
  }
  if (!strcmp(crc_name, "crc_32q") || !strcmp(crc_name, "CRC_32Q")) {
    return CRC_32Q;
  }
  if (!strcmp(crc_name, "jamcrc") || !strcmp(crc_name, "JAMCRC")) {
    return JAMCRC;
  }
  if (!strcmp(crc_name, "xfer") || !strcmp(crc_name, "XFER")) {
    return XFER;
  }
  if (!strcmp(crc_name, "crc_64") || !strcmp(crc_name, "CRC_64")) {
    return CRC_64;
  }
  if (!strcmp(crc_name, "crc_64_go_iso") ||
      !strcmp(crc_name, "CRC_64_GO_ISO")) {
    return CRC_64_GO_ISO;
  }
  if (!strcmp(crc_name, "crc_64_we") || !strcmp(crc_name, "CRC_64_WE")) {
    return CRC_64_WE;
  }
  if (!strcmp(crc_name, "crc_64_jones") || !strcmp(crc_name, "CRC_64_JONES")) {
    return CRC_64_JONES;
  }
  return CRC_INVALID;
}

static inline void hash_alg_type_to_str(bfn_hash_alg_type_t alg_type,
                                        char *alg_name) {
  if (alg_name == NULL) {
    return;
  }
  switch (alg_type) {
    case IDENTITY_DYN:
      strncpy(alg_name, "IDENTITY\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case CRC_DYN:
      strncpy(alg_name, "CRC\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case RANDOM_DYN:
      strncpy(alg_name, "RANDOM\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case XOR_DYN:
      strncpy(alg_name, "XOR\0", BF_UTILS_ALGO_NAME_LEN);
      break;
    case INVALID_DYN:
    default:
      strncpy(alg_name, "INVALID\0", BF_UTILS_ALGO_NAME_LEN);
      break;
  }
  return;
}

static inline bfn_hash_alg_type_t hash_alg_str_to_type(const char *alg_name) {
  if (alg_name == NULL) {
    return INVALID_DYN;
  }
  if (!strcmp(alg_name, "identity") || !strcmp(alg_name, "IDENTITY")) {
    return IDENTITY_DYN;
  }
  if (!strcmp(alg_name, "crc") || !strcmp(alg_name, "CRC")) {
    return CRC_DYN;
  }
  if (!strcmp(alg_name, "random") || !strcmp(alg_name, "RANDOM")) {
    return RANDOM_DYN;
  }
  if (!strcmp(alg_name, "xor") || !strcmp(alg_name, "XOR")) {
    return XOR_DYN;
  }
  return INVALID_DYN;
}

/**
 * Holds a char * value representing the required information for the standard
 * algorithms
 */
typedef struct crc_alg_info_ {
  char *crc_name;
  uint64_t poly;
  bool reverse;
  uint64_t init;
  uint64_t final_xor;
} crc_alg_info_t;

typedef struct bfn_hash_algorithm_ {
  bfn_hash_alg_type_t hash_alg;
  bool msb;
  bool extend;

  /* -- All fields below are algorithm dependent. Most of them
   *    has meaning just for the CRC algorithm. */
  bfn_crc_alg_t crc_type;  // CRC, will be Invalid if unknown crc
  int hash_bit_width;      // CRC, XOR
  bool reverse;            // CRC
  uint64_t poly;           // CRC
  uint64_t init;           // CRC
  uint64_t final_xor;      // CRC
  uint8_t **crc_matrix;    // CRC
} bfn_hash_algorithm_t;

/**
 * Verifies that the parameters of a crc algorithm are possible.  Currently
 * ensures that the polynomial is odd, and that the sizes of the init and
 * final_xor values correctly fit within the size of the polynomial
 */
bool verify_algorithm(bfn_hash_algorithm_t *alg, char **error_message);

/**
 * Given a list of parameters, this will build an algorithm to be used in a
 * hash matrix calculation
 *
 *     alg - The algorithm to be created
 *     hash_alg_type - The type of the hash algorithm.
 *     msb - whether the algorithm starts at the most significant bit
 *     extend - whether the algorithm output is repeated
 *     crc_alg - the crc algorithm type to build the crc algorithm
 */
void initialize_algorithm(bfn_hash_algorithm_t *alg,
                          bfn_hash_alg_type_t hash_alg_type,
                          bool msb,
                          bool extend,
                          bfn_crc_alg_t crc_alg);

void initialize_crc_matrix(bfn_hash_algorithm_t *alg);

void calculate_crc(bfn_hash_algorithm_t *alg,
                   uint32_t hash_output_bits,
                   uint8_t *stream,
                   uint32_t stream_len,
                   uint8_t *crc);

#ifdef __cplusplus
}
#endif

#endif /* _BF_UTILS_ALGORITHM_H_ */
