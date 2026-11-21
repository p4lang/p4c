/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

bit<16> max(in bit<16> left, in bit<16> right) {
    if (left > right)
        return left;
    return right;
}

control c(out bit<16> b) {
    apply {
        b = max(10, 12);
    }
}

control ctr(out bit<16> b);
package top(ctr _c);

top(c()) main;
