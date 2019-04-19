extern bit<32> f(in bit<32> x);
header H {
    bit<32> v;
}

control c(inout bit<32> r) {
    apply {
        H[2] h;
        h[f(2)].setValid();
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

