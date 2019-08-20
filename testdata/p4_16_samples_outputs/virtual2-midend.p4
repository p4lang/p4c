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
    @hidden action virtual2l38() {
        cntr_0.run(16w6);
    }
    @hidden table tbl_virtual2l38 {
        actions = {
            virtual2l38();
        }
        const default_action = virtual2l38();
    }
    apply {
        tbl_virtual2l38.apply();
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

