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

#ifndef BACKENDS_TOFINO_BF_ASM_POWER_CTL_H_
#define BACKENDS_TOFINO_BF_ASM_POWER_CTL_H_

#include "lib/exceptions.h"
#include "misc.h"

/* power_ctl is weirdly encoded!
 * As far as I can tell, it actually walks like this:
 * -[1:0] dimension controls hi-lo for each 8/16/32b type. In other words,
 *    [0] = 8b[31~0], 16b[47~0], 32b[31~0] and [1] = 8b[63~32], 16b[95~48], 32[63~32].
 * -Within the wider dimension, [13:0] = 112b vector, where [31:0] = control for
 *  32b section (array slice 3~0), [63:32] = control for 8b section (array slice 7~4),
 *  [111:64] = control for 16b section (array slice 13~8)
 *
 * Yes, Jay's decription of how the [1~0][13~0] translates to 224b is correct.
 * The [1~0] index discriminates phv words going to the left side alu's [0]
 * vs the right side ones [1]. Within each container size, the bottom 32
 * (or 48 for 16b) are on the left and the top half ones are on the right.
 * Pat
 *
 * CSR DESCRIPTION IS WRONG!!!
 */

template <int I>
void set_power_ctl_reg(checked_array<2, checked_array<16, ubits<I>>> &power_ctl, int reg) {
    int side = 0;
    switch (reg / (I * 8)) {
        case 1:  // 8 bit
            reg -= I * 8;
            side = reg / (I * 4);
            reg = (reg % (I * 4)) + (I * 4);
            break;
        case 2:
        case 3:  // 16 bit
            reg -= I * 16;
            side = reg / (I * 6);
            reg = (reg % (I * 6)) + (I * 8);
            break;
        case 0:  // 32 bit
            side = reg / (I * 4);
            reg = (reg % (I * 4));
            break;
        default:
            BUG("Invalid power control reg: %d", reg);
    }
    power_ctl[side][reg / I] |= 1U << reg % I;
}

#endif /* BACKENDS_TOFINO_BF_ASM_POWER_CTL_H_ */
