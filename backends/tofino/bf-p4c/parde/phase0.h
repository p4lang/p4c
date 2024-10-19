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

#ifndef BF_P4C_PARDE_PHASE0_H_
#define BF_P4C_PARDE_PHASE0_H_

#include <utility>
#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/hex.h"

using namespace P4;

std::ostream &operator<<(std::ostream &out, const IR::BFN::Phase0 *p0);

#endif /* BF_P4C_PARDE_PHASE0_H_ */
