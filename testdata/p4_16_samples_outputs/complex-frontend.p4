extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    apply {
        tmp = f(32w5);
        r = f(tmp);
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
