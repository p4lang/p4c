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
    Virtual() cntr = {
        State(16s1024) state;
        bit<16> f(in bit<16> ix) {
            return state.get(ix);
        }
    };
    apply {
        cntr.run(16w6);
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

