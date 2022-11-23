#include <core.p4>

extern E {
    E(list<bit<32>> data);
    void run();
}

control c() {
    @name("c.e") E((list<bit<32>>){32w2,32w3,32w4}) e_0;
    @hidden action list12() {
        e_0.run();
    }
    @hidden table tbl_list12 {
        actions = {
            list12();
        }
        const default_action = list12();
    }
    apply {
        tbl_list12.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
