extern bit<32> f(in bit<32> x, in bit<32> y);
control c(inout bit<32> r) {
    apply {
        r = f(f(32w5, 32w2), f(32w6, f(32w2, 32w3)));
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

