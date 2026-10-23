// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

extern Token<T> {
    Token(T value);
}

extern Box<T> {
    Box(Token<T> token);
}

parser P() {
    state start {
        transition accept;
    }
}

control C() {
    // Type inference specializes the nested constructors. MoveConstructors must
    // retain each rewritten expression until its declaration has been emitted.
    Box<bit<8>>(Token(8w2)) box;
    Box<bit<16>>(Token(16w3)) wide;
    apply { }
}

parser Parser();
control Control();
package Test(Parser p, Control c);
Test(P(), C()) main;
