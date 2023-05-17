#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct ingress_metadata_t {
    bit<1> drop;
    bit<8> egress_port;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct metadata {
    bit<1> _ing_metadata_drop0;
    bit<8> _ing_metadata_egress_port1;
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
    @name(".nop") action nop_2() {
    }
    @name(".nop") action nop_3() {
    }
    @name(".set_egress_port") action set_egress_port(@name("egress_port") bit<8> egress_port_1) {
        meta._ing_metadata_egress_port1 = egress_port_1;
    }
    @name(".ing_drop") action ing_drop() {
        meta._ing_metadata_drop0 = 1w1;
    }
    @name(".dmac") table dmac_0 {
        actions = {
            nop_2();
            set_egress_port();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr");
        }
        default_action = NoAction_3();
    }
    @name(".smac_filter") table smac_filter_0 {
        actions = {
            nop_3();
            ing_drop();
            @defaultonly NoAction_4();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("ethernet.srcAddr");
        }
        default_action = NoAction_4();
    }
    apply {
        dmac_0.apply();
        smac_filter_0.apply();
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
