/*
 * SPDX-FileCopyrightText: 2026 aeron-gh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Methods and constructors in an extern may only share a name when a call can
// be disambiguated by the number of arguments or by argument names. Two
// declarations with the same name, parameter count, and parameter names can
// never be told apart, so they are duplicates and must be rejected at the
// declaration site, even when the extern is never instantiated.
// See https://github.com/p4lang/p4c/issues/5253

extern E {
    E(bit<8> x);
    E(bit<8> x);
    void f(bit<8> a);
    void f(bit<8> a);
}
