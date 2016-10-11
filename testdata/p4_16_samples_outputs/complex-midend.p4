extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp_1;
    bit<32> tmp_2;
    action act() {
        tmp_1 = f(32w5);
        tmp_2 = f(tmp_1);
        r = tmp_2;
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
