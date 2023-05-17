extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @hidden action complex21() {
        tmp = f(32w5);
        r = f(tmp);
    }
    @hidden table tbl_complex21 {
        actions = {
            complex21();
        }
        const default_action = complex21();
    }
    apply {
        tbl_complex21.apply();
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
