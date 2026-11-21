/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern bit<32> f(in bit<32> x, out bit<16> y);

void x() {
    f(x = 1, y = _);
}

control c() {
    apply {
        x();
    }
}

control _c();
package top(_c _c);

top(c()) main;
