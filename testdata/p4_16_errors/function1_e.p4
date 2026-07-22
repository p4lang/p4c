/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
extern void f(out bit x);

control p()
{
    apply {
        f(1w1 & 1w0); // non lvalue passed to out parameter
    }
}
