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
    h_t h_1;
    bit<16> tmp_0;
    @name("MyComputeChecksum.checksum") Checksum16() checksum;
    apply {
        h_1.setValid();
        h_1.src = hdr.h.src;
        h_1.dst = hdr.h.dst;
        h_1.csum = 16w0;
        tmp_0 = checksum.get<h_t>(h_1);
        hdr.h.csum = tmp_0;
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

