#include <core.p4>

extern Fake {
    Fake();
    void call(in bit<32> data);
}

parser P() {
    bit<32> x;
    @name("P.fake") Fake() fake;
    state start {
        x = 32w0;
        fake.call(x);
        transition accept;
    }
}

control C() {
    bit<32> x_2;
    bit<32> y;
    @name("C.fake") Fake() fake_2;
    apply {
        x_2 = 32w0;
        y = x_2 + 32w1;
        fake_2.call(y);
    }
}

parser SimpleParser();
control SimpleControl();
package top(SimpleParser prs, SimpleControl ctrl);
top(P(), C()) main;

