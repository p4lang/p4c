extern void f<T>(in T data);
control proto();
package top(proto _p);
struct tuple_0 {
    bit<32> field;
    bool    field_0;
}

control c() {
    tuple_0 x_0;
    action act() {
        x_0.field = 32w10;
        x_0.field_0 = false;
        f<tuple_0>(x_0);
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
