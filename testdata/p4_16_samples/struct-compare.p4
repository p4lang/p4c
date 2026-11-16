/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Compile-time comparisons of struct constants: == / != fold component-wise
// with fields paired by name, and struct members compared with untyped
// literals are compile-time constants.

struct S {
    bit<8> a;
    bit<16> b;
}

struct N {
    S s;
    bool f;
}

const S s1 = { a = 1, b = 2 };
const S s2 = { a = 1, b = 3 };
const S s1r = { b = 2, a = 1 };  // field order differs from the declaration
const N n1 = { s = { a = 1, b = 2 }, f = true };
const N n2 = { f = true, s = { b = 2, a = 1 } };  // nested, fields reordered

const bool t1 = s1 == s2;        // false
const bool t2 = s1 != s2;        // true
const bool t3 = s1 == s1r;       // true: field order is irrelevant
const bool t4 = s1 != s1r;       // false
const bool t5 = n1 == n2;        // true: nested structs, reordered fields
const bool t6 = s1.a == 1;       // true: member compared with untyped literal
const bool t7 = s1.b != 3;       // true
const bool t8 = s1 == { 1, 2 };  // true: positional initializer

control c(out bool z);
package top(c _c);

control test(out bool zout) {
    apply {
        zout = t1 || t2 || t3 || t4 || t5 || t6 || t7 || t8;
    }
}

top(test()) main;
