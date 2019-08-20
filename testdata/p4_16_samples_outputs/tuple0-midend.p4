struct tuple_0 {
    bit<32> field;
    bool    field_0;
}

extern void f(in tuple_0 data);
control proto();
package top(proto _p);
control c() {
    tuple_0 x_0;
    @hidden action tuple0l23() {
        x_0.field = 32w10;
        x_0.field_0 = false;
        f(x_0);
        f({ 32w20, true });
    }
    @hidden table tbl_tuple0l23 {
        actions = {
            tuple0l23();
        }
        const default_action = tuple0l23();
    }
    apply {
        tbl_tuple0l23.apply();
    }
}

top(c()) main;

