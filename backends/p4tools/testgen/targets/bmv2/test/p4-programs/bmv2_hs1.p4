#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
    bit<32> index;
}

header H {
    bit<32>  a;
}

struct headers {
    ethernet_t[3] eth_hdr;
    H h;
}

struct Meta {}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr[0]);
        pkt.extract(hdr.eth_hdr[1]);
        pkt.extract(hdr.eth_hdr[2]);
        pkt.extract(hdr.h);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {

    apply {
        if (h.eth_hdr[h.h.a].index > 10)
            h.eth_hdr[h.h.a].index = 1;
    }
}

control vrfy(inout headers h, inout Meta m) { apply {
} }

control update(inout headers h, inout Meta m) { apply {} }

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in headers h) {
    apply {
        pkt.emit(h.eth_hdr[0]);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

