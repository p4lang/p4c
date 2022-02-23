header H {
    bit<8> x;
}

control c() {
    apply {
    }
}

control C();
package top(C _c);
top(c()) main;

