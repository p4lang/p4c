/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
extern If<T>
{
    T id(in T d);
}

control p1(If x) // missing type parameter
{
    apply {}
}

control p2(If<int<32>, int<32>> x) // too many type parameters
{
    apply {}
}

header h {}

control p()
{
    apply {
        h<bit> x;     // no type parameter
    }
}
