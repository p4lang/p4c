#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header h0_t {
    bit<32> f0;
    bit<32> f1;
}

struct headers {
    ethernet_t eth_hdr;
    h0_t h0;
}

struct Meta {}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h0);
        transition select(hdr.h0.f0) {
            0: invalidate;
            default: accept;
        }
    }
    state invalidate {
        hdr.h0 = (h0_t){#};
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<32>  a = 0;
    apply {
        if (h.eth_hdr.isValid())
            a = a | 1;
        if (h.h0 == (h0_t){#})
            a = a | 2;
        h.eth_hdr = (ethernet_t){#};
        h.h0.setValid();
        h.h0.f0 = a;
    }
}

control vrfy(inout headers h, inout Meta m) {
    apply {}
 }

control update(inout headers hdr, inout Meta m) { apply {
}}

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in headers h) {
    apply {
        pkt.emit(h);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

