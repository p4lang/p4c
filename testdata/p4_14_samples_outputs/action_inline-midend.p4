#include <core.p4>
#include <v1model.p4>

struct ht {
    bit<1> b;
}

struct metadata {
    @name("md") 
    ht md;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<1> y0_0;
    bit<1> y0_1;
    action NoAction_1() {
    }
    @name("b") action b_1() {
        y0_0 = meta.md.b;
        y0_0 = y0_0 + 1w1;
        meta.md.b = y0_0;
        y0_1 = meta.md.b;
        y0_1 = y0_1 + 1w1;
        meta.md.b = y0_1;
    }
    @name("t") table t_0() {
        actions = {
            b_1();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        t_0.apply();
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

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
