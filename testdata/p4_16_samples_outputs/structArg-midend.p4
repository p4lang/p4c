struct S {
    bit<32> f;
}

control caller() {
    S data_0;
    S data_1;
    action act() {
        data_1 = data_0;
        data_1.f = 32w0;
        data_0 = data_1;
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
