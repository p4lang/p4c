extern E {
    E();
    void set(in bit<32> arg);
}

control c() {
    @name("e") E() e_0;
    apply {
        e_0.set(32w10);
    }
}

control proto();
package top(proto p);
top(c()) main;
