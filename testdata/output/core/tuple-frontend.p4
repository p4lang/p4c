struct S {
    bit<32> f;
    bool    s;
}

control proto();
package top(proto _p);
control c() {
    apply {
    }
}

top(c()) main;
