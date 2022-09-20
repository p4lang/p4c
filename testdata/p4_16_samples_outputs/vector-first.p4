#include <core.p4>

extern E {
    E(Vector<bit<32>> data);
    void run();
}

control c() {
    E((Vector<bit<32>>){32w2,32w3,32w4}) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);
top(c()) main;

