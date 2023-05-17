struct tuple_0 {
    bit<32> f0;
    bool    f1;
}

extern void f(in tuple_0 data);
control proto();
package top(proto _p);
control c() {
    @name("c.x") tuple_0 x_0;
    @hidden action tuple0l23() {
        x_0.f0 = 32w10;
        x_0.f1 = false;
        f(x_0);
        f((tuple_0){f0 = 32w20,f1 = true});
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
