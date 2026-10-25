/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
extern void f(in int<32> d);

control p<T>(in T x)
{
    apply {
        f(x);
    }
}
