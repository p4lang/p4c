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
    @name("y0_0") bit<1> y0;
    @name("y0_1") bit<1> y0_2;
    @name("NoAction_1") action NoAction() {
    }
    @name("b") action b_0() {
        @name("a") {
            y0 = meta.md.b;
            y0 = y0 + 1w1;
            meta.md.b = y0;
        }
        @name("a") {
            y0_2 = meta.md.b;
            y0_2 = y0_2 + 1w1;
            meta.md.b = y0_2;
        }
    }
    @name("t") table t() {
        actions = {
            b_0();
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
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

control verifyChecksum(in headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
