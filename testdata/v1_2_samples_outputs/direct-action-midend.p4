control c(inout bit<16> y) {
    bit<32> x_0;
    @name("a") action a_0(bit<32> arg) {
        y = (bit<16>)arg;
    }
    table tbl_a() {
        actions = {
            a_0;
        }
        const default_action = a_0(x_0);
    }
    apply {
        tbl_a.apply();
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;
