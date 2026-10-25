/*
 * SPDX-FileCopyrightText: 2026 Abhishek Agarwal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// All of the overloaded extern methods and constructors below share a name with
// another declaration, but none of them is a duplicate: every pair can still be
// told apart at a call site, either by the number of arguments or by the
// argument names (argument types are never used to disambiguate overloads). None
// of these should be rejected by the duplicate-declaration check added for
// https://github.com/p4lang/p4c/issues/5253

extern E {
    // Constructors distinguished by the number of arguments.
    E(bit<8> x);
    E(bit<8> x, bit<8> y);

    // Same name and same number of parameters, but every parameter name is
    // different, so a call using argument names selects a unique method.
    void f(bit<8> a);
    void f(bit<8> b);

    // Same name, same number of parameters, and the same parameter types, with
    // all but one parameter name identical. The single differing name is still
    // enough to tell the two methods apart, so this is a legal overload rather
    // than a duplicate.
    void g(bit<8> a, bit<16> b);
    void g(bit<8> a, bit<16> c);

    // Distinguished by the number of parameters.
    void h(bit<8> a);
    void h(bit<8> a, bit<8> b);
}
