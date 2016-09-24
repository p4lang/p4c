control c(inout bit<16> y) {
    @name("x") bit<32> x;
    bit<32> arg;
    @name("a") action a_0() {
        arg = x;
        y = (bit<16>)arg;
    }
    table tbl_a() {
        actions = {
            a_0();
        }
        const default_action = a_0();
    }
    apply {
        tbl_a.apply();
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;
