#include <core.p4>

control Ing(out bit<32> a) {
    @name("Ing.cond") action cond() {
        a = 32w5;
    }
    @hidden table tbl_cond {
        actions = {
            cond();
        }
        const default_action = cond();
    }
    apply {
        tbl_cond.apply();
    }
}

control s(out bit<32> a);
package top(s e);
top(Ing()) main;
