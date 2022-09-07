#include <core.p4>

extern E {
    E(Vector<bit<32>> data);
    void run();
}

control c() {
    @name("c.e") E([ bit<32>; 32w2, 32w3, 32w4 ]) e_0;
    @hidden action vector11() {
        e_0.run();
    }
    @hidden table tbl_vector11 {
        actions = {
            vector11();
        }
        const default_action = vector11();
    }
    apply {
        tbl_vector11.apply();
    }
}

control C();
package top(C _c);
top(c()) main;

