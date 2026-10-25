/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct S { bit d; }   
const S c = { 1w1 };

control p()
{
    apply {
        S a;
        a.d = c.d;
    }
}
