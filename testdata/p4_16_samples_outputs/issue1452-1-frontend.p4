control c() {
    bit<32> x_0;
    @name("c.b") action b(out bit<32> arg) {
        arg = 32w2;
    }
    apply {
        b(x_0);
    }
}

control proto();
package top(proto p);
top(c()) main;

