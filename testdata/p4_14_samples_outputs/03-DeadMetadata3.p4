#include "/home/mbudiu/git/p4c/p4include/core.p4"
#include "/home/mbudiu/git/p4c/p4include/v1model.p4"

struct m_t {
    bit<32> f1;
    bit<32> f2;
}

struct metadata {
    @name("m") 
    m_t m;
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
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("a1") action a1() {
        meta.m.f1 = 32w1;
    }
    @name("a2") action a2() {
        meta.m.f2 = 32w2;
    }
    @name("t1") table t1() {
        actions = {
            a1;
            NoAction;
        }
        default_action = NoAction();
    }
    @name("t2") table t2() {
        actions = {
            a2;
            NoAction;
        }
        key = {
            meta.m.f1: exact;
        }
        default_action = NoAction();
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

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
