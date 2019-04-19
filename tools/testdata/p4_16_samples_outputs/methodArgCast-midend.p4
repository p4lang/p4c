extern E {
    E();
    void setValue(in bit<32> arg);
}

control c() {
    @name("c.e") E() e_0;
    @hidden action act() {
        e_0.setValue(32w10);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control proto();
package top(proto p);
top(c()) main;

