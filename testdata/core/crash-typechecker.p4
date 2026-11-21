/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//#include <core.p4>

extern bit<16> get<D>(in D data);

header H {
    bit<8>      len;
    @length((bit<32>)len * 8 - 16)
    varbit<304> data;
}

control x() {
    H h;

    apply {
        get({ h });
    }
}
