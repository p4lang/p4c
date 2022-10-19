#include <core.p4>

extern E {
    E(list<bit<32>> data);
    void run();
}

control c() {
    @name("c.e") E((list<bit<32>>){32w2,32w3,32w4}) e_0;
    @hidden action list11() {
        e_0.run();
    }
    @hidden table tbl_list11 {
        actions = {
            list11();
        }
        const default_action = list11();
    }
    apply {
        tbl_list11.apply();
    }
}

control C();
package top(C _c);
top(c()) main;

