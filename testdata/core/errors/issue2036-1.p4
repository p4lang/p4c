/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct s {
    bit<8> x;
}

extern void f(in s sarg);

control c() {
    apply {
        tuple<bit<8>> b = { 0 };
        f(b);
    }
}
