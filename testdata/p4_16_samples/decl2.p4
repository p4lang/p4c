/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control p()
{
    action b(in bit<1> x, out bit<1> y)
    {
        bit<1> z;

        {
            bit<1> x_0;
            x_0 = x;
            z = (x & x_0);
            y = z;
        }
    }

    apply {
        bit<1> x_1 = 0;
        bit<1> y_0;
        b(x_1, y_0);
    }
}

control simple();
package m(simple pipe);

.m(.p()) main;
