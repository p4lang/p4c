#include <core.p4>
#include <v1model.p4>

struct m_t {
    bit<32> f1;
    bit<32> f2;
}

struct metadata {
    bit<32> _m_f10;
    bit<32> _m_f21;
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
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name(".a1") action a1() {
        meta._m_f10 = 32w1;
    }
    @name(".a2") action a2() {
        meta._m_f21 = 32w2;
    }
    @name(".t1") table t1_0 {
        actions = {
            a1();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name(".t2") table t2_0 {
        actions = {
            a2();
            @defaultonly NoAction_3();
        }
        key = {
            meta._m_f10: exact @name("m.f1") ;
        }
        default_action = NoAction_3();
    }
    apply {
        t1_0.apply();
        t2_0.apply();
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

