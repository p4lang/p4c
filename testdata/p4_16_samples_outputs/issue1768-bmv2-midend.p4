#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct B8 {
    bit<64> bits;
}

struct headers {
}

struct metadata {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.lookahead<bit<64>>();
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @hidden action issue1768bmv2l32() {
        mark_to_drop(standard_metadata);
    }
    @hidden table tbl_issue1768bmv2l32 {
        actions = {
            issue1768bmv2l32();
        }
        const default_action = issue1768bmv2l32();
    }
    apply {
        if (standard_metadata.parser_error != error.NoError) {
            tbl_issue1768bmv2l32.apply();
        }
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
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
