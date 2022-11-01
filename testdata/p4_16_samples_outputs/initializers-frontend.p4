#include <core.p4>

extern Fake {
    Fake();
    void call(in bit<32> data);
}

parser P() {
    @name("P.x") bit<32> x_0;
    @name("P.fake") Fake() fake_0;
    state start {
        x_0 = 32w0;
        transition start_0;
    }
    state start_0 {
        fake_0.call(x_0);
        transition accept;
    }
}

control C() {
    @name("C.x") bit<32> x_1;
    @name("C.y") bit<32> y_0;
    @name("C.fake") Fake() fake_1;
    apply {
        x_1 = 32w0;
        y_0 = x_1 + 32w1;
        fake_1.call(y_0);
    }
}

parser SimpleParser();
control SimpleControl();
package top(SimpleParser prs, SimpleControl ctrl);
top(P(), C()) main;
