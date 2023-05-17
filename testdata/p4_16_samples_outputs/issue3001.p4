header H {
    bit<8> x;
}

H f() {
    H h;
    return h;
}
control c() {
    apply {
        if (f().isValid()) {
            ;
        }
    }
}

control C();
package top(C _c);
top(c()) main;
