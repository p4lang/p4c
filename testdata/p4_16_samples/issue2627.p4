/*
 * SPDX-FileCopyrightText: 2021 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct H3<T> {
    tuple<T> t;
}

struct S {
    bit<32> b;
}

const H3<S> h4 = {
    t = { { b = 0 } }
};
