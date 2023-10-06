#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<32> a;
}

struct headers {
    ethernet_t eth_hdr;
    H h;
}

struct Meta {}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        h.eth_hdr.src_addr = 0xAAAAAAAAAAAA;
        h.eth_hdr.dst_addr = 0xFFFFFFFFFFFF;
        h.eth_hdr.eth_type = 0xBBBB;
        h.h.a = 0xEEEE;
        hash(h.h.a, HashAlgorithm.csum16, 16w0, {h.eth_hdr.dst_addr, h.eth_hdr.src_addr, h.eth_hdr.eth_type}, 16w65535);
    }
}

control vrfy(inout headers h, inout Meta m) { apply {} }

control update(inout headers h, inout Meta m) { apply {} }

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in headers h) {
    apply {
        pkt.emit(h.eth_hdr);
        pkt.emit(h.h);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
