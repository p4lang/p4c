#include <core.p4>

extern E<K, V> {
    E(list<tuple<int, tuple<K, K>, V>> data);
    void run();
}

control c() {
    @name("c.e") E<bit<32>, bit<16>>((list<tuple<int, tuple<bit<32>, bit<32>>, bit<16>>>){{ 10, { 32w2, 32w0xf }, 16w3 },{ 5, { 32w0xdeadbeef, 32w0xff00ffff }, 16w5 }}) e_0;
    @hidden action vector3l18() {
        e_0.run();
    }
    @hidden table tbl_vector3l18 {
        actions = {
            vector3l18();
        }
        const default_action = vector3l18();
    }
    apply {
        tbl_vector3l18.apply();
    }
}

control C();
package top(C _c);
top(c()) main;

