control t(inout bit<32> b) {
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
