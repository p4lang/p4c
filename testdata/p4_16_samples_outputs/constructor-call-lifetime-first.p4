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
    Box<bit<8>>(Token<bit<8>>(8w2)) box;
    Box<bit<16>>(Token<bit<16>>(16w3)) wide;
    apply {
    }
}

parser Parser();
control Control();
package Test(Parser p, Control c);
Test(P(), C()) main;
