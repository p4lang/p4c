extern Fake {
    void call(in bit<32> data);
}

parser P() {
    bit<32> x_0;
    @name("fake") Fake() fake_0;
    state start {
        x_0 = 32w0;
        fake_0.call(x_0);
        transition accept;
    }
}

control C() {
    bit<32> x_1;
    bit<32> y_0;
    @name("fake") Fake() fake_1;
    action act() {
        x_1 = 32w0;
        y_0 = x_1 + 32w1;
        fake_1.call(y_0);
    }
    table tbl_act() {
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
