#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header payload_t {
    bit<8> x;
    bit<8> y;
}

struct header_t {
    payload_t payload;
}

struct metadata {
}

parser MyParser(packet_in packet, out header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<payload_t>(hdr.payload);
        transition accept;
    }
}

control MyIngress(inout header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIngress.a1") action a1() {
        hdr.payload.x = 8w0xaa;
    }
    @name("MyIngress.t1") table t1_0 {
        key = {
            hdr.payload.x: exact @name("hdr.payload.x");
        }
        actions = {
            a1();
            @defaultonly NoAction_1();
        }
        size = 1024;
        default_action = NoAction_1();
    }
    apply {
        if (hdr.payload.y == 8w0) {
            hdr.payload.x = hdr.payload.y;
            t1_0.apply();
        } else {
            t1_0.apply();
        }
        standard_metadata.egress_spec = 9w2;
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
        packet.emit<header_t>(hdr);
    }
}

control MyComputeChecksum(inout header_t hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<header_t, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
