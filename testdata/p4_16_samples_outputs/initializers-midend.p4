#include <core.p4>

extern Fake {
    Fake();
    void call(in bit<32> data);
}

parser P() {
    bit<32> x;
    @name("fake") Fake() fake;
    state start {
        x = 32w0;
        fake.call(x);
        transition accept;
    }
}

control C() {
    @name("fake") Fake() fake_2;
    @hidden action act() {
        fake_2.call(32w1);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

parser SimpleParser();
control SimpleControl();
package top(SimpleParser prs, SimpleControl ctrl);
top(P(), C()) main;

