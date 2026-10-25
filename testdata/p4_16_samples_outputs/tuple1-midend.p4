extern void f<T>(in T data);
control proto();
package top(proto _p);
struct tuple_0 {
    bit<32> f0;
    bool    f1;
}

control c() {
    @name("c.x") tuple_0 x_0;
    @hidden action tuple1l14() {
        x_0.f0 = 32w10;
        x_0.f1 = false;
        f<tuple_0>(x_0);
    }
    @hidden table tbl_tuple1l14 {
        actions = {
            tuple1l14();
        }
        const default_action = tuple1l14();
    }
    apply {
        tbl_tuple1l14.apply();
    }
}

top(c()) main;
