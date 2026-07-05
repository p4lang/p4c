/*
 * SPDX-FileCopyrightText: 2026 Nvidia Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void fn(in bit<8> c);

control c() {
    apply {
        for (bit<8> a = 15; a; a -= 1) {
            fn(a);
        }
    }
}
