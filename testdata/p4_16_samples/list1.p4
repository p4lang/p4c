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
    E((list<S>){{2, 3}, {4, 5}}) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);

top(c()) main;
