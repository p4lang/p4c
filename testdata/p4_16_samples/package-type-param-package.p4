/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Packages may take package types as type parameters.
 */

package Inner();
package Outer<P>(P p);

// Explicit type argument.
Outer<Inner>(Inner()) outer_explicit;

// Type argument inferred from the constructor argument.
Outer(Inner()) outer_inferred;

package Top(Outer<Inner> a, Outer<Inner> b);
Top(outer_explicit, outer_inferred) main;
