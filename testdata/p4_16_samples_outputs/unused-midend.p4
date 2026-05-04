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
    @hidden action unused29() {
        s.h.data3 = 32w0;
    }
    @hidden action unused31() {
        s.h.data1 = e_0.get<bit<32>>(s.h.data2);
    }
    @hidden table tbl_unused29 {
        actions = {
            unused29();
        }
        const default_action = unused29();
    }
    @hidden table tbl_unused31 {
        actions = {
            unused31();
        }
        const default_action = unused31();
    }
    apply {
        if (s.h.isValid()) {
            tbl_unused29.apply();
        }
        if (s.h.data2 == 32w0) {
            tbl_unused31.apply();
        }
    }
}

control cproto<T>(inout T v);
package top(cproto<_> _c);
top(c()) main;
