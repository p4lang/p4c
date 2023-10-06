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
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.retval") bit<8> retval;
    apply {
        retval = ((H){a = 8w1}).a;
        tmp = retval;
        tmp_0 = tmp;
        h.h[tmp_0].a = 8w1;
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;
