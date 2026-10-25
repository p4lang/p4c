#include <core.p4>

struct S<T> {
    T t;
}

struct S_bit32 {
    bit<32> t;
}

extern E {
    E(list<S_bit32> data);
    void run();
}

control c() {
    E((list<S_bit32>){(S_bit32){t = 32w10},(S_bit32){t = 32w5}}) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);
top(c()) main;
