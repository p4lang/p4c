/*
 * SPDX-FileCopyrightText: 2026 Nvidia Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>

header h1_t {
    bit<4>      a[4];
    bit<16>     b;
}

header h2_t {
    bit<16>     x;
    bit<16>     y;
}

struct headers_t {
    h1_t        h1;
    h2_t        h2[4];
}

struct empty_metadata_t {
}

// Pipeline section
control c(
    inout headers_t headers,
    inout empty_metadata_t user_meta
) {
    apply {
        for (bit<16> i in 0..3) {
            switch(headers.h1.a[i]) {
            0: 1: 2: {
                if (headers.h1.b > 100) {
                    headers.h2[i].x = headers.h1.b;
                    break;
                } else {
                    headers.h2[i].y = headers.h1.b;
                    continue;
                }
                headers.h2[i].setInvalid();
            }
            4: {
                headers.h2[i].x = headers.h1.b;
                break;
                headers.h2[i].setInvalid();
            } }
        }
    }
}

control c_<H, M>(inout H h, inout M m);
package top<H, M>(c_<H, M> c);
top(c()) main;
