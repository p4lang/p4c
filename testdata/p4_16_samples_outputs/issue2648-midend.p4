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
    @hidden action issue2648l36() {
        h.h[8w1].a = 8w1;
    }
    @hidden table tbl_issue2648l36 {
        actions = {
            issue2648l36();
        }
        const default_action = issue2648l36();
    }
    apply {
        tbl_issue2648l36.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;
