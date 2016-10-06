extern E {
    E();
    void set(in bit<32> arg);
}

control c() {
    @name("e") E() e;
    action act() {
        e.set(32w10);
    }
    table tbl_act() {
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
