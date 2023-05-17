#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct metadata {
}

struct headers {
    @name(".ethernet")
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_6() {
    }
    @name(".do_b") action do_b() {
    }
    @name(".do_d") action do_d() {
    }
    @name(".do_e") action do_e() {
    }
    @name(".nop") action nop() {
    }
    @name(".nop") action nop_1() {
    }
    @name(".nop") action nop_2() {
    }
    @name(".nop") action nop_3() {
    }
    @name(".nop") action nop_4() {
    }
    @name(".A") table A_0 {
        actions = {
            do_b();
            do_d();
            do_e();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr");
        }
        default_action = NoAction_1();
    }
    @name(".B") table B_0 {
        actions = {
            nop();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name(".C") table C_0 {
        actions = {
            nop_1();
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    @name(".D") table D_0 {
        actions = {
            nop_2();
            @defaultonly NoAction_4();
        }
        default_action = NoAction_4();
    }
    @name(".E") table E_0 {
        actions = {
            nop_3();
            @defaultonly NoAction_5();
        }
        default_action = NoAction_5();
    }
    @name(".F") table F_0 {
        actions = {
            nop_4();
            @defaultonly NoAction_6();
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
            default: {
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

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
