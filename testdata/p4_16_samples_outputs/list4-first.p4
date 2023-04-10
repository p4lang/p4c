#include <core.p4>

typedef int EntryPriority_t;
extern E<K, V> {
    E(list<tuple<EntryPriority_t, tuple<K, K>, V>> data);
    void run();
}

control c() {
    E<bit<32>, bit<16>>((list<tuple<int, tuple<bit<32>, bit<32>>, bit<16>>>){{ 10, { 32w2, 32w0xf }, 16w3 },{ 5, { 32w0xdeadbeef, 32w0xff00ffff }, 16w5 }}) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);
top(c()) main;
