#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<0> a;
    bit<1> b;
    bit<0> c;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h) {
    @hidden action bit0bmv2l35() {
        h.h.a = 0;
        h.h.b = 1w0;
        h.h.c = 0;
    }
    @hidden table tbl_bit0bmv2l35 {
        actions = {
            bit0bmv2l35();
        }
        const default_action = bit0bmv2l35();
    }
    apply {
        tbl_bit0bmv2l35.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;
