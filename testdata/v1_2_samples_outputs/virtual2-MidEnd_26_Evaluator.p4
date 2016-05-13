extern Virtual {
    abstract bit<16> f(in bit<16> ix);
    void run(in bit<16> ix);
}

extern State {
    State(int<16> size);
    bit<16> get(bit<16> index);
}

control c(inout bit<16> p) {
    Virtual() @name("cntr") cntr_0 = {
        State(16s1024) @name("state") state_0;
        bit<16> f(in bit<16> ix) {
            return state_0.get(ix);
        }
    };
    action act() {
        cntr_0.run(6);
    }
    table tbl_act() {
        actions = {
            act;
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;
