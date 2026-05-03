/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void f(in int<9> x, in int<9> y);

control p()
{
    action A1()
    {
        f(9s4, 9s3);
    }

    apply {}
}
