extern bit<32> f(in bit<32> x, in bit<32> y);
control c(inout bit<32> r) {
    @name("tmp") bit<32> tmp_7;
    @name("tmp_0") bit<32> tmp_8;
    @name("tmp_1") bit<32> tmp_9;
    @name("tmp_2") bit<32> tmp_10;
    @name("tmp_3") bit<32> tmp_11;
    @name("tmp_4") bit<32> tmp_12;
    @name("tmp_5") bit<32> tmp_13;
    @name("tmp_6") bit<32> tmp_14;
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
