/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern ct {
    void apply();
}

extern E<T> {
    void apply();
}

void f(tuple<ct> t)
{
    t[0].apply();
}

void g(tuple<E<bit<32>>> t)
{
    t[0].apply();
}