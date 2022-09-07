#include <core.p4>

struct S {
    bit<32> a;
    bit<32> b;
}

extern E {
    E(Vector<S> data);
    void run();
}

control c() {
    @name("c.e") E([S; { 32w2, 32w3 }, { 32w4, 32w5 }]) e_0;
    apply {
        e_0.run();
    }
}

control C();
package top(C _c);
top(c()) main;

