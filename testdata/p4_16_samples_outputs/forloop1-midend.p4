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
    @name("c.v") bit<8> v_1;
    @name("c.x") bit<16> x_0;
    @hidden action forloop1l24() {
        hdrs.head.f1 = hdrs.head.f1 + v_0.x;
    }
    @hidden action forloop1l26() {
        v_1 = 8w0;
    }
    @hidden action forloop1l26_0() {
        x_0 = 16w1;
    }
    @hidden action forloop1l27() {
        hdrs.head.h1 = hdrs.head.h1 + x_0;
    }
    @hidden action forloop1l26_1() {
        v_1 = v_1 + 8w1;
    }
    @hidden action forloop1l26_2() {
        x_0 = x_0 << 1;
    }
    @hidden table tbl_forloop1l24 {
        actions = {
            forloop1l24();
        }
        const default_action = forloop1l24();
    }
    @hidden table tbl_forloop1l26 {
        actions = {
            forloop1l26();
        }
        const default_action = forloop1l26();
    }
    @hidden table tbl_forloop1l26_0 {
        actions = {
            forloop1l26_0();
        }
        const default_action = forloop1l26_0();
    }
    @hidden table tbl_forloop1l27 {
        actions = {
            forloop1l27();
        }
        const default_action = forloop1l27();
    }
    @hidden table tbl_forloop1l26_1 {
        actions = {
            forloop1l26_1();
        }
        const default_action = forloop1l26_1();
    }
    @hidden table tbl_forloop1l26_2 {
        actions = {
            forloop1l26_2();
        }
        const default_action = forloop1l26_2();
    }
    apply {
        for (v_0 in hdrs.stack) {
            tbl_forloop1l24.apply();
        }
        for (tbl_forloop1l26.apply(), tbl_forloop1l26_0.apply(); v_1 < hdrs.head.cnt; tbl_forloop1l26_1.apply(), tbl_forloop1l26_2.apply()) {
            tbl_forloop1l27.apply();
        }
    }
}

top<headers_t>(c()) main;
