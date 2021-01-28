#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<32> a;
}

struct Headers {
    ethernet_t eth_hdr;
    H[2]       h;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<32> tmp;
    @name("ingress.do_something") action do_something(inout bit<32> val) {
        if (h.eth_hdr.eth_type == 16w1) {
            tmp = 32w1;
        } else {
            tmp = 32w2;
        }
        val = tmp;
    }
    apply {
        do_something(h.h[0].a);
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

