struct S {
    bit<32> f;
}

control caller() {
    @name("data") S data;
    @name("data_1") S data_2;
    action act() {
        data.f = 32w0;
        data_2 = data;
        data_2.f = 32w0;
        data = data_2;
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
