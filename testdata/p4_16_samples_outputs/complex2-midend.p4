extern bit<32> f(in bit<32> x);
header H {
    bit<32> v;
}

control c(inout bit<32> r) {
    H[2] h;
    bit<32> tmp_1;
    bit<32> tmp_2;
    action act() {
        tmp_1 = f(32w2);
        tmp_2 = tmp_1;
        h[tmp_2].setValid();
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

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
