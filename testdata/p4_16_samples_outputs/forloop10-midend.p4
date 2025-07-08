#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> f1;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  cnt;
}

struct headers_t {
    t1 head;
}

control c(inout headers_t hdrs) {
    @name("c.sum") bit<32> sum_0;
    @name("c.i") bit<32> i_0;
    @hidden action forloop10l23() {
        i_0 = i_0 + 32w4294967295;
    }
    @hidden action forloop10l21() {
        sum_0 = sum_0 + i_0;
    }
    @hidden action forloop10l25() {
        hdrs.head.cnt = hdrs.head.cnt + 8w1;
    }
    @hidden action forloop10l18() {
        sum_0 = 32w1;
    }
    @hidden table tbl_forloop10l18 {
        actions = {
            forloop10l18();
        }
        const default_action = forloop10l18();
    }
    @hidden table tbl_forloop10l21 {
        actions = {
            forloop10l21();
        }
        const default_action = forloop10l21();
    }
    @hidden table tbl_forloop10l23 {
        actions = {
            forloop10l23();
        }
        const default_action = forloop10l23();
    }
    @hidden table tbl_forloop10l25 {
        actions = {
            forloop10l25();
        }
        const default_action = forloop10l25();
    }
    apply {
        tbl_forloop10l18.apply();
        @unroll for (i_0 = 32w5; i_0 != 32w0; ) {
            tbl_forloop10l21.apply();
            if (sum_0 == 32w10) {
                tbl_forloop10l23.apply();
            }
            tbl_forloop10l25.apply();
        }
    }
}

top<headers_t>(c()) main;
