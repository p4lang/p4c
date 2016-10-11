struct S {
    bit<32> f;
    bool    s;
}

control proto();
package top(proto _p);
control c() {
    tuple<bit<32>, bool> x;
    tuple<bit<32>, bool> y;
    S z;
    action act() {
        x = { 32w10, false };
        y = x;
        z = y;
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

top(c()) main;
