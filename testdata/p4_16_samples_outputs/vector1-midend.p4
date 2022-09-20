#include <core.p4>

struct S {
    bit<32> a;
    bit<32> b;
}

extern E {
    E(Vector<S> data);
    void run();
}

control c() {
    @name("c.e") E((Vector<S>){(S){a = 32w2,b = 32w3},(S){a = 32w4,b = 32w5}}) e_0;
    @hidden action vector1l16() {
        e_0.run();
    }
    @hidden table tbl_vector1l16 {
        actions = {
            vector1l16();
        }
        const default_action = vector1l16();
    }
    apply {
        tbl_vector1l16.apply();
    }
}

control C();
package top(C _c);
top(c()) main;

