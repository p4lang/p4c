extern bit<32> f(in bit<32> x, in bit<32> y);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_1") bit<32> tmp_1;
    @name("c.tmp_3") bit<32> tmp_3;
    @hidden action complex1l21() {
        tmp = f(32w5, 32w2);
        tmp_1 = f(32w2, 32w3);
        tmp_3 = f(32w6, tmp_1);
        r = f(tmp, tmp_3);
    }
    @hidden table tbl_complex1l21 {
        actions = {
            complex1l21();
        }
        const default_action = complex1l21();
    }
    apply {
        tbl_complex1l21.apply();
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

