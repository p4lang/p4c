#include <core.p4>

extern void f(bit<32> a = 0, bit<32> b);
extern E {
    E(bit<32> x = 0);
    void f(in bit<16> z = 2);
}

control c()(bit<32> binit = 4) {
    E() e;
    apply {
        f(b = binit);
        e.f();
    }
}

control ctrl();
package top(ctrl _c);

top(c()) main;
