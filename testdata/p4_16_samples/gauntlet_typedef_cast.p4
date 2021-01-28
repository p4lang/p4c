#include <core.p4>
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

typedef bit<32> dummy_def;
const dummy_def prefix = 2348810240;

struct Headers {
    ethernet_t eth_hdr;
    H h;
}

parser p(packet_in packet, out Headers hdr) {
    state start {
        packet.extract(hdr.eth_hdr);
        transition select(hdr.eth_hdr.dst_addr[31:32 - 8]) {
            prefix[31:32 - 8]: do_parse;
            default: accept;
        }
    }
    state do_parse {
        packet.extract(hdr.h);
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

