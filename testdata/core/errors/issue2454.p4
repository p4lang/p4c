/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control C() { apply { } }

void f<T>(T t) {}

control D() {
    C() c;
    apply {
        f(c);
    }
}