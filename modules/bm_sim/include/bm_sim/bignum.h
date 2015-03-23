#ifndef _BM_BIGNUM_H_
#define _BM_BIGNUM_H_

// will need to implement this ourselves if we do not use Boost
#include <boost/multiprecision/gmp.hpp>

#include <gmp.h>

namespace bignum {

  using namespace boost::multiprecision;

  typedef number<gmp_int> Bignum;
  
  inline size_t export_bytes(char *dst, size_t size, const Bignum &src) {
    size_t count;
    mpz_export(dst, &count, 1, size, 1, 0, src.backend().data());
    return count;
  }

  inline void import_bytes(Bignum &dst, const char *src, size_t size) {
    mpz_import(dst.backend().data(), 1, 1, size, 1, 0, src);
  }

}

#endif
