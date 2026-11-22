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
            16w0x8100: p1;
            16w0x9100: p1;
            16w0xa100 &&& 16w0xefff: p1;
            (16w0xc100 &&& 16w0xefff): p1;
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
            hdr.h.f1: ternary @name("hdr.h.f1");
        }
        actions = {
            hit();
            @defaultonly NoAction();
        }
        const entries = {
                        16w0x101 : hit(16w1);
                        16w0x101 &&& 16w0x505 : hit(16w5);
                        default : hit(16w0);
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

test<my_headers_t, my_meta_t>(MyIngress()) main;
