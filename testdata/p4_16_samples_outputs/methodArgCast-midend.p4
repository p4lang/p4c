extern E {
    E();
    void setValue(in bit<32> arg);
}

control c() {
    @name("c.e") E() e_0;
    @hidden action methodArgCast15() {
        e_0.setValue(32w10);
    }
    @hidden table tbl_methodArgCast15 {
        actions = {
            methodArgCast15();
        }
        const default_action = methodArgCast15();
    }
    apply {
        tbl_methodArgCast15.apply();
    }
}

control proto();
package top(proto p);
top(c()) main;
