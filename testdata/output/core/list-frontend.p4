#include <core.p4>

extern E {
    E(list<bit<32>> data);
    void run();
}

control c() {
    @name("c.e") E((list<bit<32>>){32w2,32w3,32w4}) e_0;
    apply {
        e_0.run();
    }
}

control C();
package top(C _c);
top(c()) main;
