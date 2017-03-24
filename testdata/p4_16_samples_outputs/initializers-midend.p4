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
    bit<32> x_2;
    bit<32> y;
    @name("fake") Fake() fake_2;
    action act() {
        x_2 = 32w0;
        y = 32w1;
        fake_2.call(32w1);
    }
    table tbl_act {
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
