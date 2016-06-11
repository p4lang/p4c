extern E {
    void set(in bit<32> arg);
}

control c() {
    E() e;
    apply {
        e.set(32w10);
    }
}

control proto();
package top(proto p);
top(c()) main;
