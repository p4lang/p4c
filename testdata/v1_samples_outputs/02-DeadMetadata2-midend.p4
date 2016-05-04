#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

struct m_t {
    bit<32> f1;
}

struct metadata {
    @name("m") 
    m_t m;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        meta.m.f1 = 32w2;
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    @name("a1") action a1_0() {
        meta.m.f1 = 32w1;
    }
    @name("t1") table t1_0() {
        actions = {
            a1_0;
            NoAction_0;
        }
        default_action = NoAction_0();
    }
    apply {
        t1_0.apply();
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

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
