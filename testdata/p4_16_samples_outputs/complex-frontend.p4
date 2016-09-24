extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp;
    bit<32> tmp_0;
    apply {
        tmp = f(32w5);
        tmp_0 = f(tmp);
        r = tmp_0;
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
