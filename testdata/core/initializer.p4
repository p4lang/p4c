/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern bit<32> random(bit<5> logRange);

control ingress(out bit<32> field_d_32) {
    action action_1() {
        {
            bit<32> tmp = random((bit<5>)24);
            field_d_32[31:8] = tmp[31:8];
        }
    }
    apply {
    }
}
