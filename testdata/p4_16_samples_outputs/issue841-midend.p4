#include <core.p4>
#define V1MODEL_VERSION 20180101
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
        packet.extract<h_t>(hdr.h);
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
    @name("MyComputeChecksum.h") h_t h_0;
    @name("MyComputeChecksum.checksum") Checksum16() checksum_0;
    apply {
        h_0.setValid();
        h_0.src = hdr.h.src;
        h_0.dst = hdr.h.dst;
        h_0.csum = 16w0;
        hdr.h.csum = checksum_0.get<h_t>(h_0);
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
