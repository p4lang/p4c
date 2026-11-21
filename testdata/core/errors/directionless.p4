/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control p()
{
    action a(bit x0, out bit y0)
    {
        bit x = x0;
        y0 = x0 & x;
    }

    action b(bit x, out bit y)
    {
        bit z;
        a(x, z);
        a(z & z, y);
    }

    apply {
        bit x;
        bit y;
        b(x, y);
    }
}

control Simple();
package m(Simple pipe);

m(p()) main;
