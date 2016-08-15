extern bit<32> f(in bit<32> x, in bit<32> y);
control c(inout bit<32> r) {
    bit<32> tmp;
    bit<32> tmp_0;
    bit<32> tmp_1;
    action act() {
        tmp = f(32w5, 32w2);
        tmp_0 = f(32w2, 32w3);
        tmp_1 = f(32w6, tmp_0);
        r = f(tmp, tmp_1);
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
