extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    apply {
        r = f(f(5));
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

