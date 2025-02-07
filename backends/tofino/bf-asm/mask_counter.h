/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_ASM_MASK_COUNTER_H_
#define BF_ASM_MASK_COUNTER_H_

#include <limits.h>

#include "bitvec.h"

class MaskCounter {
    unsigned mask, val;
    bool oflo;

 public:
    explicit MaskCounter(unsigned m) : mask(m), val(0), oflo(false) {}
    explicit operator bool() const { return !oflo; }
    operator unsigned() const { return val; }
    bool operator==(const MaskCounter &a) const { return val == a.val && oflo == a.oflo; }
    MaskCounter &operator++() {
        val = ((val | ~mask) + 1) & mask;
        if (val == 0) oflo = true;
        return *this;
    }
    MaskCounter operator++(int) {
        MaskCounter tmp(*this);
        ++*this;
        return tmp;
    }
    MaskCounter &operator--() {
        val = (val - 1) & mask;
        if (val == mask) oflo = true;
        return *this;
    }
    MaskCounter operator--(int) {
        MaskCounter tmp(*this);
        --*this;
        return tmp;
    }
    MaskCounter &clear() {
        val = 0;
        oflo = false;
        return *this;
    }
    MaskCounter &overflow(bool v = true) {
        oflo = v;
        return *this;
    }
};

#endif /* BF_ASM_MASK_COUNTER_H_ */
