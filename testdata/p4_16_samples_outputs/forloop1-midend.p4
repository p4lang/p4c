#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> f1;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  cnt;
}

header t2 {
    bit<32> x;
}

struct headers_t {
    t1    head;
    t2[8] stack;
}

control c(inout headers_t hdrs) {
    @name("c.v") t2 v_0;
    @hidden action forloop1l24() {
        hdrs.head.f1 = hdrs.head.f1 + v_0.x;
    }
    @hidden action forloop1l27() {
        hdrs.head.h1 = hdrs.head.h1 + 16w1;
    }
    @hidden table tbl_forloop1l24 {
        actions = {
            forloop1l24();
        }
        const default_action = forloop1l24();
    }
    @hidden table tbl_forloop1l27 {
        actions = {
            forloop1l27();
        }
        const default_action = forloop1l27();
    }
    apply {
        for (v_0 in hdrs.stack) {
            tbl_forloop1l24.apply();
        }
        for (; 8w0 < hdrs.head.cnt; ) {
            tbl_forloop1l27.apply();
        }
    }
}

top<headers_t>(c()) main;
