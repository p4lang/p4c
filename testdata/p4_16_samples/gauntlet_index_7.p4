#include <core.p4>

bit<1> max(in bit<1> val) {
    return val;
}
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8>  a;
}

struct Headers {
    ethernet_t eth_hdr;
    H[2]  h;
    bit<1> id;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        transition accept;
    }
}

bit<8> assign_id(inout bit<1> id) {
    id = 0;
    return 1;
}

control ingress(inout Headers h) {

    apply {
        h.h[max(h.id)].a = assign_id(h.id);
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

