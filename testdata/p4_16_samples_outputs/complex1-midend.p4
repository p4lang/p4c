extern bit<32> f(in bit<32> x, in bit<32> y);
control c(inout bit<32> r) {
    bit<32> tmp_7;
    bit<32> tmp_8;
    bit<32> tmp_9;
    bit<32> tmp_10;
    bit<32> tmp_11;
    bit<32> tmp_12;
    bit<32> tmp_13;
    bit<32> tmp_14;
    action act() {
        tmp_7 = f(32w5, 32w2);
        tmp_8 = tmp_7;
        tmp_9 = 32w6;
        tmp_10 = f(32w2, 32w3);
        tmp_11 = tmp_10;
        tmp_12 = f(tmp_9, tmp_11);
        tmp_13 = tmp_12;
        tmp_14 = f(tmp_8, tmp_13);
        r = tmp_14;
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
