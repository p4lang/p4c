#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header header_t {
}

struct metadata {
}

parser MyParser(packet_in packet, out header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control MyIngress(inout header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyVerifyChecksum(inout header_t hdr, inout metadata meta) {
    apply {
    }
}

control MyEgress(inout header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyDeparser(packet_out packet, in header_t hdr) {
    apply {
    }
}

control MyComputeChecksum(inout header_t hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<header_t, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
