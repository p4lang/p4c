#include <core.p4>
#include <v1model.p4>

struct ht {
    bit<1> b;
}

struct metadata {
    @name(".md") 
    ht md;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".a") action a_0(inout bit<1> y0_0) {
        y0_0 = y0_0 + 1w1;
    }
    @name(".b") action b_0() {
        a_0(meta.md.b);
        a_0(meta.md.b);
    }
    @name(".t") table t_0 {
        actions = {
            b_0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
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

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

