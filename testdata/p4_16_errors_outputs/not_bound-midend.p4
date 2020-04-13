struct headers {
}

struct metadata {
}

#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<9> port;
    @name("ingress.set_nhop") action set_nhop() {
        standard_metadata.egress_spec = port;
    }
    @hidden table tbl_set_nhop {
        actions = {
            set_nhop();
        }
        const default_action = set_nhop();
    }
    apply {
        tbl_set_nhop.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

