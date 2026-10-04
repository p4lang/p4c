/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */

// A header comment stays before its declaration.
control C() {
    action empty() { /* inside an empty body */ }
    action set_value(inout bit<8> x) {
        x =/* inline value */1; // trailing statement comment
}
    apply {
        bit<8> x;
        set_value(x);
    }
}
// The final comment also belongs to the source file.
