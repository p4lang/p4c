extern void f<T>(in T data);
control proto();
package top(proto _p);
struct tuple_0 {
    bit<32> field;
    bool    field_0;
}

control c() {
    tuple_0 x;
    @hidden action tuple2l23() {
        x.field = 32w10;
        x.field_0 = false;
        f<tuple_0>(x);
    }
    @hidden table tbl_tuple2l23 {
        actions = {
            tuple2l23();
        }
        const default_action = tuple2l23();
    }
    apply {
        tbl_tuple2l23.apply();
    }
}

top(c()) main;

