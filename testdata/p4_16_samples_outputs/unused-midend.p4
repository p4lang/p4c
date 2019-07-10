#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
    bit<32> data3;
}

struct S {
    Header h;
}

extern E {
    E();
    bit<32> get<T>(in T b);
}

control c(inout S s) {
    bit<32> tmp;
    @name("c.e") E() e_0;
    @hidden action act() {
        s.h.data3 = 32w0;
    }
    @hidden action act_0() {
        tmp = e_0.get<bit<32>>(s.h.data2);
        s.h.data1 = tmp;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        if (s.h.isValid()) {
            tbl_act.apply();
        }
        if (s.h.data2 == 32w0) {
            tbl_act_0.apply();
        }
    }
}

control cproto<T>(inout T v);
package top(cproto<_> _c);
top(c()) main;

