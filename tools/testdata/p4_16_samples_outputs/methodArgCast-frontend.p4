extern E {
    E();
    void setValue(in bit<32> arg);
}

control c() {
    @name("c.e") E() e_0;
    apply {
        e_0.setValue(32w10);
    }
}

control proto();
package top(proto p);
top(c()) main;

