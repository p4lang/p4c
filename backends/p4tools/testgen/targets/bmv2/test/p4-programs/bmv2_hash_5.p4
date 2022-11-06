#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<32> a;
    bit<32> b;
    bit<32> c;
    bit<32> d;
    bit<32> e;
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
        h.h.setValid();
        // Taken from https://github.com/p4lang/behavioral-model/blob/27c235944492ef55ba061fcf658b4d8102d53bd8/tests/test_calculations.cpp#L332
        hash(h.h.a, HashAlgorithm.csum16, 16w0, {8w0x45, 8w0x00, 8w0x00, 8w0x73, 8w0x00, 8w0x00,
                                        8w0x40, 8w0x00, 8w0x40, 8w0x11, 8w0x00, 8w0x00,
                                        8w0xc0, 8w0xa8, 8w0x00, 8w0x01, 8w0xc0, 8w0xa8,
                                        8w0x00, 8w0xc7}, 16w65535);
        hash(h.h.b, HashAlgorithm.csum16, 16w0, {8w0, 8w1, 8w0, 8w1, 8w0, 8w1}, 16w65535);
        hash(h.h.c, HashAlgorithm.csum16, 16w0, {8w1, 8w0, 8w1, 8w0, 8w1, 8w0}, 16w65535);
        hash(h.h.d, HashAlgorithm.csum16, 16w0, {16w1, 16w1, 16w1}, 16w65535);
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
