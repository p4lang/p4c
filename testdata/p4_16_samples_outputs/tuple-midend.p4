struct S {
    bit<32> f;
    bool    s;
}

control proto();
package top(proto _p);
struct tuple_0 {
    bit<32> field;
    bool    field_0;
}

control c() {
    tuple_0 x;
    action act() {
        x.field = 32w10;
        x.field_0 = false;
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
