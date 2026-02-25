header h_t {
    bit<4> a;
}

control C() {
    apply {
    }
}

control proto();
package top(proto p);
top(C()) main;
