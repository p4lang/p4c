#include <core.p4>

control Ingress<H, M>(inout H hdr, inout M meta);
package test<H, M>(Ingress<H, M> ig);
header h_h {
    bit<16> f1;
    bit<8>  f2;
}

struct my_headers_t {
    h_h h;
}

struct my_meta_t {
}

parser prs(inout my_headers_t hdr) {
    state start {
        transition select(hdr.h.f1) {
            0x8100: p1;
            0x9100: p1;
            0xa100 &&& 0xefff: p1;
            (0xc100 &&& 0xefff): p1;
            default: p1;
            (default): reject;
        }
    }
    state p1 {
        transition accept;
    }
}

control MyIngress(inout my_headers_t hdr, inout my_meta_t meta) {
    action hit(bit<16> p) {
        hdr.h.f1 = p;
    }
    table t {
        key = {
            hdr.h.f1: ternary;
        }
        actions = {
            hit;
        }
        const entries = {
                        16w0x101 : hit(1);
                        16w0x101 &&& 16w0x505 : hit(5);
                        default : hit(0);
        }
    }
    apply {
        t.apply();
    }
}

test(MyIngress()) main;
