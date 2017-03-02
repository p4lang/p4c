control t(inout bit<32> b) {
    bit<32> c1_tmp_0;
    bit<32> c2_tmp_0;
    action act() {
        b = b + 32w1;
        b = b + 32w1;
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

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;
