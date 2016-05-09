header H {
    bit<32> a;
    bit<32> b;
}

struct S {
    H       h1;
    H       h2;
    bit<32> c;
}

parser p() {
    S s_0;
    state start {
        s_0.h1.setValid(false);
        s_0.h2.setValid(false);
        s_0.c = 32w0;
        transition accept;
    }
}

parser empty();
package top(empty e);
top(p()) main;
