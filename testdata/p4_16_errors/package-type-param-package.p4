/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Packages may not be used as type arguments to non-package generics.
 */

package Inner();

struct S<T> {
    bit<8> x;
}

const S<Inner> s = { x = 0 };
