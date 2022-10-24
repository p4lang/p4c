header H {
    bit<8> x;
}

control c() {
    @name("c.h") H h_0;
    apply {
        h_0.setInvalid();
    }
}

control C();
package top(C _c);
top(c()) main;
