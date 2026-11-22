/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

struct pvs_t {
    @match(ternary) bit<32> f32;
    @match(1+1) bit<16> f16;
}
