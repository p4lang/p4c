#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/v1model.p4"

struct metadata_t {
    bit<1> field_1_1_1;
    bit<1> field_2_1_1;
}

struct metadata {
    @name("md") 
    metadata_t md;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasReturned_0 = false;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("action_1_1") action action_1_1(bit<1> value) {
        bool hasReturned_2 = false;
        meta.md.field_1_1_1 = value;
        meta.md.field_2_1_1 = 1w1;
    }
    @name("dmac") table dmac() {
        actions = {
            action_1_1;
            NoAction;
        }
        key = {
        }
        default_action = NoAction();
    }

    apply {
        bool hasReturned_1 = false;
        dmac.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        bool hasReturned_3 = false;
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasReturned_4 = false;
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        bool hasReturned_5 = false;
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
