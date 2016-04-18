#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/v1model.p4"

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
        bool hasExited = false;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("a1") action a1() {
        bool hasReturned_0 = false;
        meta.m.f1 = 32w1;
    }
    @name("a2") action a2() {
        bool hasReturned_1 = false;
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
        bool hasExited_0 = false;
        t1.apply();
        t2.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        bool hasExited_1 = false;
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasExited_2 = false;
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        bool hasExited_3 = false;
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
