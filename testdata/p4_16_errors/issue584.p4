/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <v1model.p4>

typedef bit<16> Hash;

control p();
package top(p _p);

control c() {
    apply {
        bit<16> var;
        bit<32> hdr = 0;

        hash(var, HashAlgorithm.crc16, 16w0, hdr, 0xFFFF);
    }
}

top(c()) main;
