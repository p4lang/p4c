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
    @name("C.tmp") Token<bit<8>>(8w2) tmp;
    @name("C.box") Box<bit<8>>(tmp) box_0;
    @name("C.tmp_0") Token<bit<16>>(16w3) tmp_0;
    @name("C.wide") Box<bit<16>>(tmp_0) wide_0;
    apply {
    }
}

parser Parser();
control Control();
package Test(Parser p, Control c);
Test(P(), C()) main;
