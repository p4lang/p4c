/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bf-p4c/mau/mau_spec.h"
#include "input_xbar.h"

int TofinoIXBarSpec::getExactOrdBase(int group) const {
    return group * Tofino::IXBar::EXACT_BYTES_PER_GROUP;
}

int TofinoIXBarSpec::getTernaryOrdBase(int group) const {
    return Tofino::IXBar::EXACT_GROUPS * Tofino::IXBar::EXACT_BYTES_PER_GROUP +
        (group / 2) * Tofino::IXBar::TERNARY_BYTES_PER_BIG_GROUP +
        (group % 2) * (Tofino::IXBar::TERNARY_BYTES_PER_GROUP + 1 /* mid byte */);
}
