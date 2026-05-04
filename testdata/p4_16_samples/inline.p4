/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control p(out bit y)
{
    action a(in bit x0, out bit y0)
    {
        bit x = x0;
        y0 = x0 & x;
    }

    action b(in bit x, out bit y)
    {
        bit z;
        a(x, z);
        a(z & z, y);
    }

    apply {
        bit x = 1;
        b(x, y);
    }
}

control simple(out bit y);
package m(simple pipe);

m(p()) main;
