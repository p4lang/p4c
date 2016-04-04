#ifndef P4C_LIB_BITOPS_H_
#define P4C_LIB_BITOPS_H_

#include <limits.h>
#include <gmpxx.h>
#include "exceptions.h"

static inline unsigned bitcount(unsigned v) {
    unsigned rv = 0;
    while (v) { v &= v-1; ++rv; }
    return rv; }
static inline unsigned bitcount(mpz_class value) {
    mpz_class v = value;
    if (sgn(v) < 0)
        BUG("bitcount of negative number %1%", value);
    unsigned rv = 0;
    while (v != 0) { v &= v-1; ++rv; }
    return rv; }
static inline int ceil_log2(unsigned v) {
    if (!v) return -1;
    for (int rv = 0; rv < static_cast<int>(CHAR_BIT*sizeof(unsigned)); rv++)
        if ((1U << rv) >= v) return rv;
    return CHAR_BIT*sizeof(unsigned); }
static inline int floor_log2(unsigned v) {
    int rv = -1;
    while (v) { rv++; v >>= 1; }
    return rv; }

#endif /* P4C_LIB_BITOPS_H_ */
