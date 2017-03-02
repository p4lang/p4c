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
    bit<32> tmp_1;
    bool tmp_2;
    @name("e") E() e;
    action act() {
        s.h.data3 = 32w0;
    }
    action act_0() {
        tmp_1 = e.get<bit<32>>(s.h.data2);
        s.h.data1 = tmp_1;
    }
    action act_1() {
        tmp_2 = s.h.data2 == 32w0;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_0() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_1() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        if (s.h.isValid()) 
            tbl_act.apply();
        tbl_act_0.apply();
        if (s.h.data2 == 32w0) {
            tbl_act_1.apply();
        }
    }
}

control cproto<T>(inout T v);
package top(cproto<_> _c);
top(c()) main;
