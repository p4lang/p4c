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
    @hidden action initializers29() {
        fake_1.call(32w1);
    }
    @hidden table tbl_initializers29 {
        actions = {
            initializers29();
        }
        const default_action = initializers29();
    }
    apply {
        tbl_initializers29.apply();
    }
}

parser SimpleParser();
control SimpleControl();
package top(SimpleParser prs, SimpleControl ctrl);
top(P(), C()) main;
