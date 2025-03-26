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

#include "depositfield.h"

namespace DepositField {

RotateConstant discoverRotation(int32_t val, int containerSize, int32_t tooLarge,
                                int32_t tooSmall) {
    int32_t containerMask = ~(UINT64_MAX << containerSize);
    int32_t signBit = 1U << (containerSize - 1);
    unsigned rotate = 0;
    for (/*rotate*/; rotate < containerSize; ++rotate) {
        if (val > tooSmall && val < tooLarge) break;
        // Reverse the rotate-right to discover encoding.
        int32_t rotBit = (val >> (containerSize - 1)) & 1;
        val = ((val << 1) | rotBit) & containerMask;
        val |= (val & signBit) ? ~containerMask : 0;
    }
    // If a solution has not been found, val is back to where it started.
    rotate %= containerSize;
    return RotateConstant{rotate, val};
}

}  // namespace DepositField
