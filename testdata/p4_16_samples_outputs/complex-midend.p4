extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp_2;
    bit<32> tmp_4;
    @hidden action act() {
        tmp_2 = f(32w5);
        tmp_4 = f(tmp_2);
        r = tmp_4;
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

