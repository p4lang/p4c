struct S {
    bit<32> f;
}

control caller() {
    S data;
    S data_2;
    action act() {
        data.f = 32w0;
        data_2.f = data.f;
        data_2.f = 32w0;
        data.f = data_2.f;
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
