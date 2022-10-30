#include <core.p4>

extern Fake {
    Fake();
    void call(in bit<32> data);
}

parser P() {
    @name("P.fake") Fake() fake_0;
    state start {
        fake_0.call(32w0);
        transition accept;
    }
}

control C() {
    @name("C.fake") Fake() fake_1;
    @hidden action initializers38() {
        fake_1.call(32w1);
    }
    @hidden table tbl_initializers38 {
        actions = {
            initializers38();
        }
        const default_action = initializers38();
    }
    apply {
        tbl_initializers38.apply();
    }
}

parser SimpleParser();
control SimpleControl();
package top(SimpleParser prs, SimpleControl ctrl);
top(P(), C()) main;
