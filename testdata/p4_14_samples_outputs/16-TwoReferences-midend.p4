#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct metadata {
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_1() {
    }
    action NoAction_2() {
    }
    action NoAction_3() {
    }
    action NoAction_4() {
    }
    action NoAction_5() {
    }
    action NoAction_6() {
    }
    @name("do_b") action do_b() {
    }
    @name("do_d") action do_d() {
    }
    @name("do_e") action do_e() {
    }
    @name("nop") action nop() {
    }
    @name("nop") action nop_1() {
    }
    @name("nop") action nop_2() {
    }
    @name("nop") action nop_3() {
    }
    @name("nop") action nop_4() {
    }
    @name("A") table A_0() {
        actions = {
            do_b();
            do_d();
            do_e();
            NoAction_1();
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = NoAction_1();
    }
    @name("B") table B_0() {
        actions = {
            nop();
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("C") table C_0() {
        actions = {
            nop_1();
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    @name("D") table D_0() {
        actions = {
            nop_2();
            NoAction_4();
        }
        default_action = NoAction_4();
    }
    @name("E") table E_0() {
        actions = {
            nop_3();
            NoAction_5();
        }
        default_action = NoAction_5();
    }
    @name("F") table F_0() {
        actions = {
            nop_4();
            NoAction_6();
        }
        default_action = NoAction_6();
    }
    apply {
        switch (A_0.apply().action_run) {
            do_b: {
                B_0.apply();
                C_0.apply();
            }
            do_d: {
                D_0.apply();
                C_0.apply();
            }
            do_e: {
                E_0.apply();
            }
        }

        F_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
