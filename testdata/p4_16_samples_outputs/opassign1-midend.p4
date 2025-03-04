#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
@test_keep_opassign header t1 {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
}

struct headers_t {
    t1 head;
}

control c(inout headers_t hdrs) {
    @hidden action opassign1l23() {
        hdrs.head.f1 += hdrs.head.f2;
        hdrs.head.f1 *= hdrs.head.f3;
        hdrs.head.f1 %= hdrs.head.f4;
        hdrs.head.f1 <<= hdrs.head.b1;
        hdrs.head.f1 >>= hdrs.head.b2;
    }
    @hidden table tbl_opassign1l23 {
        actions = {
            opassign1l23();
        }
        const default_action = opassign1l23();
    }
    apply {
        tbl_opassign1l23.apply();
    }
}

top<headers_t>(c()) main;
