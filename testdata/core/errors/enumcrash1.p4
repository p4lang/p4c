/*
 * SPDX-FileCopyrightText: 2025 Nvidia Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum Foo { A, B, C };
extern void testfn<T>(T arg);

action test() {
    testfn(arg = Foo.D);
}
