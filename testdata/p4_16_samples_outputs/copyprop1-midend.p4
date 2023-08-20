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
    @hidden action copyprop1l39() {
        hdr.payload.x = hdr.payload.y;
    }
    @hidden action copyprop1l44() {
        standard_metadata.egress_spec = 9w2;
    }
    @hidden table tbl_copyprop1l39 {
        actions = {
            copyprop1l39();
        }
        const default_action = copyprop1l39();
    }
    @hidden table tbl_copyprop1l44 {
        actions = {
            copyprop1l44();
        }
        const default_action = copyprop1l44();
    }
    apply {
        if (hdr.payload.y == 8w0) {
            tbl_copyprop1l39.apply();
            t1_0.apply();
        } else {
            t1_0.apply();
        }
        tbl_copyprop1l44.apply();
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
        packet.emit<payload_t>(hdr.payload);
    }
}

control MyComputeChecksum(inout header_t hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<header_t, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
