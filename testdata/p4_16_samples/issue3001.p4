/*
 * Copyright 2021 Intel Corporation
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header H {
    bit<8> x;
}

H f() {
    H h;
    return h;
}

control c()
{
    apply {
        if (f().isValid()) ;
    }
}

control C();
package top(C _c);

top(c()) main;
