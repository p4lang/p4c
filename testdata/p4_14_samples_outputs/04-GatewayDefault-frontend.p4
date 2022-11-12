#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct ingress_metadata_t {
    bit<8>  drop;
    bit<8>  egress_port;
    bit<8>  f1;
    bit<16> f2;
    bit<32> f3;
    bit<64> f4;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct metadata {
    @name(".ing_metadata")
    ingress_metadata_t ing_metadata;
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
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name(".nop") action nop() {
    }
    @name(".e_t1") table e_t1_0 {
        actions = {
            nop();
            @defaultonly NoAction_2();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("ethernet.srcAddr");
        }
        default_action = NoAction_2();
    }
    apply {
        e_t1_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_6() {
    }
    @name(".nop") action nop_2() {
    }
    @name(".nop") action nop_3() {
    }
    @name(".nop") action nop_4() {
    }
    @name(".nop") action nop_5() {
    }
    @name(".ing_drop") action ing_drop() {
        meta.ing_metadata.drop = 8w1;
    }
    @name(".set_f1") action set_f1(@name("f1") bit<8> f1_1) {
        meta.ing_metadata.f1 = f1_1;
    }
    @name(".set_f2") action set_f2(@name("f2") bit<16> f2_1) {
        meta.ing_metadata.f2 = f2_1;
    }
    @name(".set_f2") action set_f2_1(@name("f2") bit<16> f2_2) {
        meta.ing_metadata.f2 = f2_2;
    }
    @name(".set_f3") action set_f3(@name("f3") bit<32> f3_1) {
        meta.ing_metadata.f3 = f3_1;
    }
    @name(".set_f3") action set_f3_1(@name("f3") bit<32> f3_2) {
        meta.ing_metadata.f3 = f3_2;
    }
    @name(".set_egress_port") action set_egress_port(@name("egress_port") bit<8> egress_port_1) {
        meta.ing_metadata.egress_port = egress_port_1;
    }
    @name(".set_f4") action set_f4(@name("f4") bit<64> f4_1) {
        meta.ing_metadata.f4 = f4_1;
    }
    @name(".i_t1") table i_t1_0 {
        actions = {
            nop_2();
            ing_drop();
            set_f1();
            set_f2();
            set_f3();
            set_egress_port();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr");
        }
        default_action = NoAction_3();
    }
    @name(".i_t2") table i_t2_0 {
        actions = {
            nop_3();
            set_f2_1();
            @defaultonly NoAction_4();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr");
        }
        default_action = NoAction_4();
    }
    @name(".i_t3") table i_t3_0 {
        actions = {
            nop_4();
            set_f3_1();
            @defaultonly NoAction_5();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr");
        }
        default_action = NoAction_5();
    }
    @name(".i_t4") table i_t4_0 {
        actions = {
            nop_5();
            set_f4();
            @defaultonly NoAction_6();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr");
        }
        default_action = NoAction_6();
    }
    apply {
        switch (i_t1_0.apply().action_run) {
            nop_2: {
                i_t2_0.apply();
            }
            set_egress_port: {
                i_t3_0.apply();
            }
            default: {
                i_t4_0.apply();
            }
        }
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
