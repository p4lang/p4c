#include <core.p4>

extern E {
    E(list<bit<32>> data);
    void run();
}

control c() {
    E((list<bit<32>>){2, 3, 4}) e;
    bit<32> list;  // check that 'list' identifier is allowed
    apply {
        e.run();
    }
}

control C();
package top(C _c);

top(c()) main;
