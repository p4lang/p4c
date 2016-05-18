/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef P4C_LIB_BITOPS_H_
#define P4C_LIB_BITOPS_H_

#include <limits.h>
#include "gmputil.h"
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
