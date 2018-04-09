#include <core.p4>
#include <v1model.p4>

struct m_t {
    bit<32> f1;
    bit<32> f2;
}

struct metadata {
    @name(".m") 
    m_t m;
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
    @name(".a1") action a1() {
        meta.m.f1 = 32w1;
    }
    @name(".a2") action a2() {
        meta.m.f2 = 32w2;
    }
    @name(".t1") table t1 {
        actions = {
            a1;
        }
    }
    @name(".t2") table t2 {
        actions = {
            a2;
        }
        key = {
            meta.m.f1: exact;
        }
    }
    apply {
        t1.apply();
        t2.apply();
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

