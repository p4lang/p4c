struct tuple_0 {
    bit<32> field;
    bool    field_0;
}

extern void f(in tuple_0 data);
control proto();
package top(proto _p);
control c() {
    tuple_0 x;
    @hidden action act() {
        x.field = 32w10;
        x.field_0 = false;
        f(x);
        f({ 32w20, true });
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

top(c()) main;

