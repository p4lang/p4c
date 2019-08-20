extern void f<T>(in T data);
control proto();
package top(proto _p);
struct tuple_0 {
    bit<32> field;
    bool    field_0;
}

control c() {
    tuple_0 x_0;
    @hidden action tuple1l23() {
        x_0.field = 32w10;
        x_0.field_0 = false;
        f<tuple_0>(x_0);
    }
    @hidden table tbl_tuple1l23 {
        actions = {
            tuple1l23();
        }
        const default_action = tuple1l23();
    }
    apply {
        tbl_tuple1l23.apply();
    }
}

top(c()) main;

