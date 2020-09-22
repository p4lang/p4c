#include <core.p4>
#include <v1model.p4>

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
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h.next);
        pkt.extract(hdr.h.next);
        transition accept;
    }
}

action dummy(inout Headers val) {}
control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action simple_action() {
        if (h.eth_hdr.src_addr != h.eth_hdr.dst_addr) {
            h.h[0].a = 2;
        } else {
            h.h[1].a = 1;
        }
    }
    apply {
        simple_action();
    }
}
control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {b.emit(h);} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
