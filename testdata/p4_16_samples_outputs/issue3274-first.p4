extern bit<32> f(in bit<32> x, out bit<16> y);
bit<32> x() {
    return f(x = 32w1, y = _);
}
control c(out bit<32> r) {
    apply {
        r = x();
    }
}

control _c(out bit<32> r);
package top(_c _c);
top(c()) main;

