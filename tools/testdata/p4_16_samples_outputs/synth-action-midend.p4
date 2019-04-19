control c(inout bit<32> x) {
    @hidden action act() {
        x = 32w10;
        x = 32w12;
        x = 32w6;
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

control n(inout bit<32> x);
package top(n _n);
top(c()) main;

