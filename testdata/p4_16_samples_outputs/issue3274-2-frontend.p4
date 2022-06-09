extern bit<32> f(in bit<32> x, out bit<16> y);
control c() {
    @name("c.arg") bit<16> arg;
    apply {
        f(x = 32w1, y = arg);
    }
}

control _c();
package top(_c _c);
top(c()) main;

