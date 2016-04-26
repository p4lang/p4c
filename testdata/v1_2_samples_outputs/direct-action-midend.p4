control c(inout bit<16> y) {
    @name("x") bit<32> x_0_0;
    action a(bit<32> arg) {
        y = (bit<16>)arg;
    }
    table tbl_a() {
        actions = {
            a;
        }
        const default_action = a(x_0_0);
    }
    apply {
        tbl_a.apply();
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;
