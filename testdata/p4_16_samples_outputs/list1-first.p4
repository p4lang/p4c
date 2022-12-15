#include <core.p4>

struct S {
    bit<32> a;
    bit<32> b;
}

extern E {
    E(list<S> data);
    void run();
}

control c() {
    E((list<S>){(S){a = 32w2,b = 32w3},(S){a = 32w4,b = 32w5}}) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);
top(c()) main;
