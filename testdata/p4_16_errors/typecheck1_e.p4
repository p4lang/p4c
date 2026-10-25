/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control p()
{
    apply {
        bit<32>  a;
        int<32>  b;
        bit<32>  c;
        bool     d;

        // all of these are illegal operations
        b = ~b;
        c = ~c;
        d = ~d;

        a = a + a;
        a = a - a;

        d = d + d;
        d = d - d;

        a = !a;
        b = !b;
    }
}
