#include <core.p4>
#include <v1model.p4>

header h_t {
    bit<32> src;
    bit<32> dst;
    bit<16> csum;
}

struct metadata {
}

struct headers {
    h_t h;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.h);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    Checksum16() checksum;
    h_t h = { hdr.h.src, hdr.h.dst, 16w0 };
    apply {
        hdr.h.csum = checksum.get(h);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

