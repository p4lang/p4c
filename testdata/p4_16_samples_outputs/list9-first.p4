#include <core.p4>

struct S<T> {
    T t;
}

struct S_0 {
    bit<32> t;
}

extern E {
    E(list<S_0> data);
    void run();
}

control c() {
    E((list<S_0>){(S_0){t = 32w10},(S_0){t = 32w5}}) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);
top(c()) main;
