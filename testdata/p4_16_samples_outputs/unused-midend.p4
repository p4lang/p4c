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
    @name("c.e") E() e_0;
    @hidden action unused38() {
        s.h.data3 = 32w0;
    }
    @hidden action unused40() {
        s.h.data1 = e_0.get<bit<32>>(s.h.data2);
    }
    @hidden table tbl_unused38 {
        actions = {
            unused38();
        }
        const default_action = unused38();
    }
    @hidden table tbl_unused40 {
        actions = {
            unused40();
        }
        const default_action = unused40();
    }
    apply {
        if (s.h.isValid()) {
            tbl_unused38.apply();
        }
        if (s.h.data2 == 32w0) {
            tbl_unused40.apply();
        }
    }
}

control cproto<T>(inout T v);
package top(cproto<_> _c);
top(c()) main;
