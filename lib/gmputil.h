#ifndef _LIB_GMPUTIL_H_
#define _LIB_GMPUTIL_H_

#include <gmpxx.h>

namespace Util {

// Useful functions for manipulating GMP values
// (arbitrary-precision values)

mpz_class ripBits(mpz_class &value, int bits);

struct BitRange {
    unsigned  lowIndex;
    unsigned  highIndex;
    mpz_class value;
};

// Find a consecutive scan of 1 bits at the "bottom"
BitRange findOnes(const mpz_class &value);

mpz_class cvtInt(const char *s, unsigned base);
mpz_class shift_left(const mpz_class &v, unsigned bits);
mpz_class shift_right(const mpz_class &v, unsigned bits);
// Convert a slice [m:l] into a mask
mpz_class maskFromSlice(unsigned m, unsigned l);
mpz_class mask(unsigned bits);
}  // namespace Util

#endif /* _LIB_GMPUTIL_H_ */
