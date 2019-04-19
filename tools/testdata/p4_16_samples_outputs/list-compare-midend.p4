struct S {
    bit<32> l;
    bit<32> r;
}

control c(out bool z);
package top(c _c);
struct tuple_0 {
    bit<32> field;
    bit<32> field_0;
}

control test(out bool zout) {
    @hidden action act() {
        zout = true;
        zout = true;
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

top(test()) main;

