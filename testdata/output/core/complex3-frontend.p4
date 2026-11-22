extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_0") bit<32> tmp_0;
    @name("c.tmp_1") bit<32> tmp_1;
    @name("c.tmp_2") bit<32> tmp_2;
    apply {
        tmp_0 = f(32w4);
        tmp = tmp_0;
        tmp_1 = f(32w5);
        tmp_2 = tmp + tmp_1;
        r = tmp_2;
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
