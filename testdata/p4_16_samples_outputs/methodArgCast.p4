extern E {
    E();
    void setValue(in bit<32> arg);
}

control c() {
    E() e;
    apply {
        e.setValue(10);
    }
}

control proto();
package top(proto p);
top(c()) main;

