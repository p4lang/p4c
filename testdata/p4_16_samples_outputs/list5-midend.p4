#include <core.p4>

extern E<K, V> {
    E(list<tuple<bit<32>, tuple<K, K>, V>> data);
    void run();
}

control c() {
    @name("c.e") E<bit<32>, bit<16>>((list<tuple<bit<32>, tuple<bit<32>, bit<32>>, bit<16>>>){{ 32w10, { 32w2, 32w0xf }, 16w3 },{ 32w5, { 32w0xdeadbeef, 32w0xff00ffff }, 16w5 }}) e_0;
    @hidden action list5l24() {
        e_0.run();
    }
    @hidden table tbl_list5l24 {
        actions = {
            list5l24();
        }
        const default_action = list5l24();
    }
    apply {
        tbl_list5l24.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
