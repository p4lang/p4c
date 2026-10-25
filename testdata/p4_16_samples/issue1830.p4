/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void foo<T>(in T x) {}

void bar() {
    foo(true);
}

void main() {
    foo(8w0);
}
