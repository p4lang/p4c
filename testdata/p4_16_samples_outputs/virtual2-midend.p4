extern Virtual {
    Virtual();
    void run(in bit<16> ix);
    @synchronous(run) abstract bit<16> f(in bit<16> ix);
}

extern State {
    State(int<16> size);
    bit<16> get(in bit<16> index);
}

control c(inout bit<16> p) {
    @name("c.cntr") Virtual() cntr_0 = {
        @name("c.state") State(16s1024) state_0;
        bit<16> f(in bit<16> ix) {
            bit<16> tmp;
            tmp = state_0.get(ix);
            return tmp;
        }
    };
    @hidden action act() {
        cntr_0.run(16w6);
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

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

