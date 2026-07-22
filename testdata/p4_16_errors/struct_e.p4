/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
struct s {}

control p()
{
    apply {
        s v;
        bit b;

        v.data = 1w0; // no such field
        b.data = 5;   // no such field
    }
}
