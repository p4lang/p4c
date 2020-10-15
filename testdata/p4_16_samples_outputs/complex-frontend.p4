extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_0") bit<32> tmp_0;
    apply {
        tmp = f(32w5);
        tmp_0 = tmp;
        r = f(tmp_0);
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

