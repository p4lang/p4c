#include <core.p4>
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<32>  a;
}

struct Headers {
    ethernet_t eth_hdr;
    H[2]  h;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h) {
    action do_something(inout bit<32> val) {
        val = (h.eth_hdr.eth_type == 1 ? 32w1 : 32w2);
    }
    apply {
        do_something(h.h[0].a);
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

