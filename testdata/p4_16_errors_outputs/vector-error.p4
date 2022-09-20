#include <core.p4>

extern E {
    E(Vector<bit<32>> data);
    void run();
}

control c() {
    E((Vector<bit<16>>){ 2, 3, 4 }) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);
top(c()) main;

