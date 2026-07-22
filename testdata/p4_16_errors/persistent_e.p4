/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control p()(bit y)
{
    apply {}
}

control q(in bit z)
{
    p(z) p1;  // argument is not constant
    apply {
        p1.apply();
    }
}

control simple(in bit z);

package m(simple s);

m(q()) main;
