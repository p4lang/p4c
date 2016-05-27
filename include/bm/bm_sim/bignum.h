/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_BM_SIM_BIGNUM_H_
#define BM_BM_SIM_BIGNUM_H_

// will need to implement this ourselves if we do not use Boost
#include <boost/multiprecision/gmp.hpp>

#include <gmp.h>

namespace bm {

namespace bignum {

  using boost::multiprecision::gmp_int;
  using boost::multiprecision::number;

  typedef number<gmp_int> Bignum;

  inline size_t export_bytes(char *dst, size_t size, const Bignum &src) {
    size_t count;
    mpz_export(dst, &count, 1, size, 1, 0, src.backend().data());
    return count;
  }

  inline size_t export_size_in_bytes(const Bignum &src) {
    return (mpz_sizeinbase(src.backend().data(), 2) + 7) / 8;
  }

  inline void import_bytes(Bignum *dst, const char *src, size_t size) {
    mpz_import(dst->backend().data(), 1, 1, size, 1, 0, src);
  }

  inline int test_bit(const Bignum &v, size_t index) {
    return mpz_tstbit(v.backend().data(), index);
  }

  inline void clear_bit(Bignum *v, size_t index) {
    return mpz_clrbit(v->backend().data(), index);
  }

}  // namespace bignum

}  // namespace bm

#endif  // BM_BM_SIM_BIGNUM_H_
