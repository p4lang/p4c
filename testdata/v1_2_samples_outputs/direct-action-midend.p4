control c(inout bit<16> y) {
    bit<32> x_0;
    @name("a") action a(in bit<32> arg_0) {
        y = (bit<16>)arg_0;
    }
    table tbl_a() {
        actions = {
            a(x_0);
        }
        const default_action = a();
    }
    apply {
        tbl_a.apply();
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;
