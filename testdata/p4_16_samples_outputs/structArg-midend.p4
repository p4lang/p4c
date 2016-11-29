struct S {
    bit<32> f;
}

control caller() {
    S data;
    action act() {
        data.f = 32w0;
        data.f = 32w0;
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

control none();
package top(none n);
top(caller()) main;
