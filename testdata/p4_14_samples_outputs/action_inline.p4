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
    @name(".a") action a(inout bit<1> y0) {
        y0 = y0 + 1w1;
    }
    @name(".b") action b() {
        a(meta.md.b);
        a(meta.md.b);
    }
    @name(".t") table t {
        actions = {
            b;
        }
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

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

