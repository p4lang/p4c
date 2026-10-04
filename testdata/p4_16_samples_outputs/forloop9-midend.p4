#include <core.p4>

control generic < M >(inout M m);
package top < M >(generic<M> c);
header t1 {
    bit<32> f1;
    bit<16> h1;
    bit<8> b1;
    bit<8> cnt;
}

struct headers_t {
    t1 head;
}

control c(inout headers_t hdrs) {
    @name("c.i") bit<32> i_0;
    @name("c.j") bit<32> j_0;
    @hidden action forloop9l27() {
        i_0 = i_0 + 32w1;
    }
    @hidden action forloop9l24() {
        i_0 = 32w0;
    }
    @hidden action forloop9l32() {
        hdrs.head.h1 = hdrs.head.h1 + 16w1;
    }
    @hidden table tbl_forloop9l24 {
        actions = {
            forloop9l24();
        }
        const default_action = forloop9l24();
    }
    @hidden table tbl_forloop9l27 {
        actions = {
            forloop9l27();
        }
        const default_action = forloop9l27();
    }
    @hidden table tbl_forloop9l32 {
        actions = {
            forloop9l32();
        }
        const default_action = forloop9l32();
    }
    apply {
        tbl_forloop9l24.apply();
        @no_unroll for (j_0 = 32w0; j_0 < 32w10; j_0 = j_0 + 32w1) {
        tbl_forloop9l27.apply();
        }
        @unroll for (; i_0 < 32w20;) {
            tbl_forloop9l32.apply();
        }
    }
}

top < headers_t >(c()) main;
