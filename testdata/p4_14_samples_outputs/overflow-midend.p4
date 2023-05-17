#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct metadata_t {
    bit<1> field_1_1_1;
    bit<1> field_2_1_1;
}

struct metadata {
    bit<1> _md_field_1_1_10;
    bit<1> _md_field_2_1_11;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".action_1_1") action action_0(@name("value") bit<1> value_1) {
        meta._md_field_1_1_10 = value_1;
        meta._md_field_2_1_11 = 1w1;
    }
    @name(".dmac") table dmac_0 {
        actions = {
            action_0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        dmac_0.apply();
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
