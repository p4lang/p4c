extern Virtual {
    Virtual();
    abstract bit<16> f(in bit<16> ix);
    void run(in bit<16> ix);
}

extern State {
    State(int<16> size);
    bit<16> get(in bit<16> index);
}

control c(inout bit<16> p) {
    @name("c.cntr") Virtual() cntr = {
        @name("c.state") State(16s1024) state_1;
        bit<16> f(in bit<16> ix) {
            bit<16> tmp_0;
            tmp_0 = state_1.get(ix);
            return tmp_0;
        }
    };
    apply {
        cntr.run(16w6);
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

