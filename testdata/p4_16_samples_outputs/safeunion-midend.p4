#include <core.p4>

enum Either_Tag {
    t,
    None
}

struct Either {
    bit<32>    t;
    Either_Tag tag;
}

control c(out bool o) {
    @hidden action safeunion14() {
        o = false;
    }
    @hidden table tbl_safeunion14 {
        actions = {
            safeunion14();
        }
        const default_action = safeunion14();
    }
    apply {
        tbl_safeunion14.apply();
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

