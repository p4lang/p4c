#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct h {
    bit<1> b;
}

struct metadata {
    bit<1> _m_b0;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".x") action _x() {
    }
    @name(".t") table _t_0 {
        actions = {
            _x();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        if (meta._m_b0 == 1w1) {
            _t_0.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
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
