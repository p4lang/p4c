/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct s {
    bit<8> x;
}

extern void f(in tuple<bit<8>> a, in s sarg);

control c() {
    apply {
        f({0}, {0});
    }
}
