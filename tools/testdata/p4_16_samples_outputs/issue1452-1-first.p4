control c() {
    bit<32> x;
    action b(out bit<32> arg) {
        arg = 32w2;
    }
    apply {
        b(x);
    }
}

control proto();
package top(proto p);
top(c()) main;

