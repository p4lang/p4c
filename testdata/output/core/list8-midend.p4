#include <core.p4>

extern E {
    E(list<list<bit<32>>> data);
    void run();
}

control c() {
    @name("c.e") E((list<list<bit<32>>>){(list<bit<32>>){32w10,32w6,32w3},(list<bit<32>>){32w5,32w2}}) e_0;
    @hidden action list8l22() {
        e_0.run();
    }
    @hidden table tbl_list8l22 {
        actions = {
            list8l22();
        }
        const default_action = list8l22();
    }
    apply {
        tbl_list8l22.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
