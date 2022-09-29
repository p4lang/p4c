#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

parser p(packet_in pkt, out Headers hdr) {
    @name("p.tmp_0") ethernet_t tmp_0;
    bit<112> tmp_1;
    state start {
        tmp_1 = pkt.lookahead<bit<112>>();
        tmp_0.setValid();
        tmp_0.dst_addr = tmp_1[111:64];
        tmp_0.src_addr = tmp_1[63:16];
        tmp_0.eth_type = tmp_1[15:0];
        transition select(tmp_1[15:0]) {
            16w0xdead: parse_hdrs;
            default: accept;
        }
    }
    state parse_hdrs {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
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

