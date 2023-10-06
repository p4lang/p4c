#include <core.p4>

extern E<K, V> {
    E(list<tuple<int, tuple<K, K>, V>> data);
    void run();
}

control c() {
    @name("c.e") E<bit<32>, bit<16>>((list<tuple<int, tuple<bit<32>, bit<32>>, bit<16>>>){{ 10, { 32w2, 32w0xf }, 16w3 },{ 5, { 32w0xdeadbeef, 32w0xff00ffff }, 16w5 }}) e_0;
    @hidden action list4l18() {
        e_0.run();
    }
    @hidden table tbl_list4l18 {
        actions = {
            list4l18();
        }
        const default_action = list4l18();
    }
    apply {
        tbl_list4l18.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
