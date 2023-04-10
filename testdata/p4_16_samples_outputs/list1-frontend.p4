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
    @name("c.e") E((list<S>){(S){a = 32w2,b = 32w3},(S){a = 32w4,b = 32w5}}) e_0;
    apply {
        e_0.run();
    }
}

control C();
package top(C _c);
top(c()) main;
