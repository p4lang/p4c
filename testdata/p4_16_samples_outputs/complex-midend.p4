extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp;
    bit<32> tmp_1;
    @hidden action act() {
        tmp = f(32w5);
        tmp_1 = f(tmp);
        r = tmp_1;
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

