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
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h) {
    @hidden action gauntlet_index_10l29() {
        h.h[h.eth_hdr.eth_type].setValid();
        h.h[h.eth_hdr.eth_type].a = 8w2;
    }
    @hidden table tbl_gauntlet_index_10l29 {
        actions = {
            gauntlet_index_10l29();
        }
        const default_action = gauntlet_index_10l29();
    }
    apply {
        tbl_gauntlet_index_10l29.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

