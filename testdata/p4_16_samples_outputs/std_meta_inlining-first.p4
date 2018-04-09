#include <core.p4>
#include <v1model.p4>

struct headers_t {
}

struct metadata_t {
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
    }
}

action send_to_cpu(inout standard_metadata_t standard_metadata) {
    standard_metadata.egress_spec = 9w64;
}
control ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    table t0 {
        key = {
            standard_metadata.ingress_port: ternary @name("standard_metadata.ingress_port") ;
        }
        actions = {
            send_to_cpu(standard_metadata);
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t0.apply();
    }
}

control egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control computeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

