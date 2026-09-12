/*
 * SPDX-FileCopyrightText: 2024 P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser P<H>(in H h, in bit<8> x = 0);

package top<H>(P<H> p);

parser MyParser(in bit<8> h, in bit<8> x) {
    state start {
        transition accept;
    }
}

top(MyParser()) main;
