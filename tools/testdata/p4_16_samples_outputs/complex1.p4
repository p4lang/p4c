extern bit<32> f(in bit<32> x, in bit<32> y);
control c(inout bit<32> r) {
    apply {
        r = f(f(5, 2), f(6, f(2, 3)));
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

