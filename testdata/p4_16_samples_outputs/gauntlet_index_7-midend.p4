#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

struct Headers {
    ethernet_t eth_hdr;
    H[2]       h;
    bit<1>     id;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h) {
    @name("ingress.tmp_0") bit<1> tmp_0;
    @hidden action gauntlet_index_7l39() {
        tmp_0 = h.id;
        h.id = 1w0;
        h.h[tmp_0].a = 8w1;
    }
    @hidden table tbl_gauntlet_index_7l39 {
        actions = {
            gauntlet_index_7l39();
        }
        const default_action = gauntlet_index_7l39();
    }
    apply {
        tbl_gauntlet_index_7l39.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

