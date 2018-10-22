bit<32> min(in bit<32> a, in bit<32> b) {
    return (a > b ? b : a);
}
control c(inout bit<32> x) {
    apply {
        x = min(min(x, x + 1), min(x, x - 1));
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;

