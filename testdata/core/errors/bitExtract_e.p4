/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
struct S {}

control p()
{
    apply {
        S s;

        bit<4> dt;

        bit<4> x = s[3:0];   // wrong type for s
        bit<8> y = dt[7:0];  // too many bits
        bit<4> z = dt[7:4];  // too many bits
    }
}
