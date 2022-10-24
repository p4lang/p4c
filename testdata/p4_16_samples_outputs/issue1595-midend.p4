#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct Parsed_packet {
    Ethernet_h ethernet;
}

struct metadata_t {
    bit<4> a;
    bit<4> b;
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit<Ethernet_h>(hdr.ethernet);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<Ethernet_h>(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("cIngress.a1") action a1() {
        hdr.ethernet.srcAddr = 48w1;
    }
    @name("cIngress.a2") action a2() {
        hdr.ethernet.srcAddr[47:40] = 8w2;
    }
    @name("cIngress.a3") action a3() {
        hdr.ethernet.srcAddr[47:40] = 8w3;
    }
    @name("cIngress.a4") action a4() {
        hdr.ethernet.srcAddr[47:40] = 8w4;
    }
    @name("cIngress.t1") table t1_0 {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr");
        }
        actions = {
            a1();
            a2();
            a3();
            a4();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action issue1595l76() {
        hdr.ethernet.srcAddr[39:32] = 8w2;
    }
    @hidden action issue1595l77() {
        hdr.ethernet.srcAddr[39:32] = 8w3;
    }
    @hidden action issue1595l78() {
        hdr.ethernet.srcAddr[39:32] = 8w4;
    }
    @hidden action issue1595l79() {
        hdr.ethernet.srcAddr[39:32] = 8w5;
    }
    @hidden table tbl_issue1595l76 {
        actions = {
            issue1595l76();
        }
        const default_action = issue1595l76();
    }
    @hidden table tbl_issue1595l77 {
        actions = {
            issue1595l77();
        }
        const default_action = issue1595l77();
    }
    @hidden table tbl_issue1595l78 {
        actions = {
            issue1595l78();
        }
        const default_action = issue1595l78();
    }
    @hidden table tbl_issue1595l79 {
        actions = {
            issue1595l79();
        }
        const default_action = issue1595l79();
    }
    apply {
        switch (t1_0.apply().action_run) {
            a1: {
            }
            a2: {
                tbl_issue1595l76.apply();
            }
            a3: {
                tbl_issue1595l77.apply();
            }
            a4: {
                tbl_issue1595l78.apply();
            }
            NoAction_1: {
                tbl_issue1595l79.apply();
            }
        }
    }
}

control cEgress(inout Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout Parsed_packet hdr, inout metadata_t meta) {
    apply {
    }
}

control uc(inout Parsed_packet hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<Parsed_packet, metadata_t>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;
