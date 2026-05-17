/*
 * SPDX-FileCopyrightText: 2024 Nvidia Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void fn(in bit<8> c);

control c() {
    apply {
        for (bit<8> a in 16w0..16w15) {
            fn(a);
        }
    }
}
