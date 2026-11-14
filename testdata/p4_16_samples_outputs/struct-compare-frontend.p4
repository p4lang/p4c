struct S {
    bit<8>  a;
    bit<16> b;
}

struct N {
    S    s;
    bool f;
}

control c(out bool z);
package top(c _c);
control test(out bool zout) {
    apply {
        zout = true;
    }
}

top(test()) main;
