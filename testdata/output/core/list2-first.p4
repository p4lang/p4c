#include <core.p4>

extern E<T, S> {
    E(list<tuple<T, S>> data);
    void run();
}

control c() {
    E<bit<32>, bit<16>>((list<tuple<bit<32>, bit<16>>>){{ 32w2, 16w3 },{ 32w4, 16w5 }}) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);
top(c()) main;
