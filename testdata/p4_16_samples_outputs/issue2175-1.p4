struct data {
    bit<16> a;
}

extern Virtual {
    Virtual();
    abstract bit<16> f();
    abstract void g(inout data ix);
}

control c(inout bit<16> p) {
    Virtual() cntr = {
        bit<16> f() {
            return 1;
        }
        void g(inout data x) {
        }
    };
    apply {
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;
