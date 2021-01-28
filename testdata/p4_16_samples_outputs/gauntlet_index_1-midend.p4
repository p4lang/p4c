#include <core.p4>

header I {
    bit<3> a;
}

header H {
    bit<32> a;
}

struct Headers {
    I    i;
    H[2] h;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<3> tmp_1;
    @hidden action gauntlet_index_1l4() {
        tmp_1 = 3w1;
    }
    @hidden action gauntlet_index_1l4_0() {
        tmp_1 = h.i.a;
    }
    @hidden action gauntlet_index_1l31() {
        h.h[tmp_1].a = 32w1;
    }
    @hidden table tbl_gauntlet_index_1l4 {
        actions = {
            gauntlet_index_1l4();
        }
        const default_action = gauntlet_index_1l4();
    }
    @hidden table tbl_gauntlet_index_1l4_0 {
        actions = {
            gauntlet_index_1l4_0();
        }
        const default_action = gauntlet_index_1l4_0();
    }
    @hidden table tbl_gauntlet_index_1l31 {
        actions = {
            gauntlet_index_1l31();
        }
        const default_action = gauntlet_index_1l31();
    }
    apply {
        if (h.i.a > 3w1) {
            tbl_gauntlet_index_1l4.apply();
        } else {
            tbl_gauntlet_index_1l4_0.apply();
        }
        tbl_gauntlet_index_1l31.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

