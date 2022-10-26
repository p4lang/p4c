#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct metadata {
}

struct headers {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t std_meta) {
    state start {
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t std_meta) {
    @name("MyIngress.output") bit<8> output_0;
    apply {
        hash<bit<8>, bit<8>, bit<8>, bit<8>>(output_0, HashAlgorithm.crc16, 8w0, 8w42, 8w255);
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
