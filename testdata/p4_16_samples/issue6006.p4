/*
 * SPDX-FileCopyrightText: 2026 Pantherun Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void foo(in bit<8> value = 7, @optional in bit<8> unused);

control Ingress() {
    apply {
        foo();
        foo(unused = 11);

        foo(value = 13);
        foo(value = 17, unused = 19);

        foo(23);
        foo(29, 31);
    }
}

control C();
package Test(C ingress);
Test(Ingress()) main;
