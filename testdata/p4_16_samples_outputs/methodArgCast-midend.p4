extern E {
    E();
    void setValue(in bit<32> arg);
}

control c() {
    @name("c.e") E() e_0;
    @hidden action methodArgCast24() {
        e_0.setValue(32w10);
    }
    @hidden table tbl_methodArgCast24 {
        actions = {
            methodArgCast24();
        }
        const default_action = methodArgCast24();
    }
    apply {
        tbl_methodArgCast24.apply();
    }
}

control proto();
package top(proto p);
top(c()) main;
