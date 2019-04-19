control t(inout bit<32> b) {
    @hidden action act() {
        b = b + 32w1;
        b = b + 32w1;
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

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;

