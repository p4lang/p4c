extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp_0") bit<32> tmp_0;
    @name("c.tmp_1") bit<32> tmp_1;
    @hidden action complex3l12() {
        tmp_0 = f(32w4);
        tmp_1 = f(32w5);
        r = tmp_0 + tmp_1;
    }
    @hidden table tbl_complex3l12 {
        actions = {
            complex3l12();
        }
        const default_action = complex3l12();
    }
    apply {
        tbl_complex3l12.apply();
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
