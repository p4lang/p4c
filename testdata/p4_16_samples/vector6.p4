#include <core.p4>

typedef bit<32> EntryPriority_t;

extern E<K, V> {
    E(list<tuple<EntryPriority_t, tuple<K, K>, V>> data);
    void run();
}

control c() {
    E<bit<32>, bit<16>>(
        (list<tuple<bit<32>, tuple<bit<32>, bit<32>>, bit<16>>>)
        {
            {10, {2, 0xf}, 3},
            {5, {0xdeadbeef, 0xff00ffff}, 5}
        }) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);

top(c()) main;
