#include <core.p4>
#include <v1model.p4>

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

struct Meta {}

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

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action do_something(inout bit<32> val) {
        val = (h.eth_hdr.eth_type == 1 ? 32w1 : 32w2);
    }
    apply {
        do_something(h.h[0].a);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h.eth_hdr);
        pkt.emit(h.h[0]);
        pkt.emit(h.h[1]);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

