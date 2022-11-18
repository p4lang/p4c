#include <core.p4>

extern E {
    E(list<bit<32>> data);
    void run();
}

control c() {
    E((list<bit<32>>){32w2,32w3,32w4}) e;
    bit<32> list;
    apply {
        e.run();
    }
}

control C();
package top(C _c);
top(c()) main;
