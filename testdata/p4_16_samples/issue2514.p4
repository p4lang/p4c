#include "core.p4"

/************* Test Arch *****************/
control Ingress<H, M>(inout H hdr, inout M meta);
package test<H,M>(Ingress<H,M> ig);
/****************************************/

#ifndef MATCH_KIND
#define MATCH_KIND ternary
#endif

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
        transition select (hdr.h.f1) {
            0x8100  : p1; /* Works */
            (0x9100) : p1; /* Also Works */
            0xA100 &&& 0xEFFF  : p1; /* Works */
            (0xC100 &&& 0xEFFF) : p1; /* Syntax error: ',' expected */
            _ : p1; /* Works */
            (_): reject; /* Syntax error: ',' expected */
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
            hdr.h.f1 : MATCH_KIND;
#ifdef CASE_FIX
            hdr.h.f2 : MATCH_KIND;
#endif
        }
        actions = { hit; }
        const entries = {
#ifdef CASE_FIX
            (16w0x0101, 0)                     : hit(1);
            (16w0x0101 &&& 16w0x0505, 0 &&& 0) : hit(5);
            (_,_)                              : hit(0);
#else
            (16w0x0101)               : hit(1);
            (16w0x0101 &&& 16w0x0505) : hit(5);
            (_)                       : hit(0);
#endif
        }
    }

    apply {
        t.apply();
    }
}

test(MyIngress()) main;
