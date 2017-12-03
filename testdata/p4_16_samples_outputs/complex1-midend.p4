extern bit<32> f(in bit<32> x, in bit<32> y);
control c(inout bit<32> r) {
    bit<32> tmp_6;
    bit<32> tmp_8;
    bit<32> tmp_10;
    bit<32> tmp_12;
    @hidden action act() {
        tmp_6 = f(32w5, 32w2);
        tmp_8 = f(32w2, 32w3);
        tmp_10 = f(32w6, tmp_8);
        tmp_12 = f(tmp_6, tmp_10);
        r = tmp_12;
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

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

