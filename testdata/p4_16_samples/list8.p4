#include <core.p4>

extern E {
    E(list<list<bit<32>>> data);
    void run();
}

control c() {
    E(
        (list<list<bit<32>>>)
        {
            (list<bit<32>>){10, 6, 3},
            (list<bit<32>>){5, 2}
        }) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);

top(c()) main;
