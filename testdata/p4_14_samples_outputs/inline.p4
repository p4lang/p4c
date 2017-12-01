#include <core.p4>
#include <v1model.p4>

struct h {
    bit<1> b;
}

struct metadata {
    @name(".m") 
    h m;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition accept;
    }
}

control c(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".x") action x() {
    }
    @name(".t") table t {
        actions = {
            x;
        }
    }
    apply {
        if (meta.m.b == 1w1) {
            t.apply();
        }
    }
}

control d(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".c") c() c_0;
    apply {
        c_0.apply(hdr, meta, standard_metadata);
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".d") d() d_0;
    apply {
        d_0.apply(hdr, meta, standard_metadata);
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

