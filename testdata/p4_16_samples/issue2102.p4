/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

header H {
    bit<1> a;
}

struct headers {
    H h;
}

control c(inout headers hdr) {
    apply {
        if (hdr.h.a < 1) {
            hdr = hdr;
        }
    }
}

control e<T>(inout T t);
package top<T>(e<T> e);

top(c()) main;
