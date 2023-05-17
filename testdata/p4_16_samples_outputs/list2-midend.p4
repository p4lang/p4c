#include <core.p4>

extern E<T, S> {
    E(list<tuple<T, S>> data);
    void run();
}

control c() {
    @name("c.e") E<bit<32>, bit<16>>((list<tuple<bit<32>, bit<16>>>){{ 32w2, 16w3 },{ 32w4, 16w5 }}) e_0;
    @hidden action list2l11() {
        e_0.run();
    }
    @hidden table tbl_list2l11 {
        actions = {
            list2l11();
        }
        const default_action = list2l11();
    }
    apply {
        tbl_list2l11.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
