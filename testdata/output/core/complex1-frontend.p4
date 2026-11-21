extern bit<32> f(in bit<32> x, in bit<32> y);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_0") bit<32> tmp_0;
    @name("c.tmp_1") bit<32> tmp_1;
    apply {
        tmp = f(32w5, 32w2);
        tmp_0 = f(32w2, 32w3);
        tmp_1 = f(32w6, tmp_0);
        r = f(tmp, tmp_1);
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
