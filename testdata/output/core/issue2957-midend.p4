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

extern bit<8> extern_call(inout H val);
parser p(packet_in pkt, out Headers hdr) {
    @name("p.tmp_4") H tmp_3;
    state start {
        tmp_3 = hdr.h[3w1];
        extern_call(tmp_3);
        hdr.h[3w1] = tmp_3;
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h.next);
        transition accept;
    }
}

control ingress(inout Headers h) {
    apply {
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;
