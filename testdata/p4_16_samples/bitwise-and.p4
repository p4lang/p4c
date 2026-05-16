/*
 * SPDX-FileCopyrightText: 2016 Barefoot Networks, Inc.
 * Copyright 2016-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// issue #113
#include<core.p4>
#include<v1model.p4>

control C(bit<1> meta) {
    apply {
        if ((meta & 0x0) == 0) {
            digest(0, meta); // this lines causes trouble
        }
    }
}