#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
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
    @name(".send") action send(@name("port") bit<9> port) {
        standard_metadata.egress_port = port;
    }
    @name(".send") action send_0(@name("port") bit<9> port_2) {
        standard_metadata.egress_port = port_2;
    }
    @name(".discard") action discard() {
        mark_to_drop(standard_metadata);
    }
    @name(".discard") action discard_0() {
        mark_to_drop(standard_metadata);
    }
    @name(".a1") table a1_0 {
        actions = {
            send();
            discard();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr");
        }
        size = 1024;
        default_action = NoAction_1();
    }
    @name(".b1") table b1_0 {
        actions = {
            send_0();
            discard_0();
            @defaultonly NoAction_2();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr");
        }
        size = 1024;
        default_action = NoAction_2();
    }
    @name(".send") action _send_0(@name("port") bit<9> port_3) {
        standard_metadata.egress_port = port_3;
    }
    @name(".send") action _send_1(@name("port") bit<9> port_4) {
        standard_metadata.egress_port = port_4;
    }
    @name(".discard") action _discard_0() {
        mark_to_drop(standard_metadata);
    }
    @name(".discard") action _discard_1() {
        mark_to_drop(standard_metadata);
    }
    @name(".c1") table _c1 {
        actions = {
            _send_0();
            _discard_0();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr");
        }
        size = 1024;
        default_action = NoAction_3();
    }
    @name(".c2") table _c2 {
        actions = {
            _send_1();
            _discard_1();
            @defaultonly NoAction_4();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr");
        }
        size = 1024;
        default_action = NoAction_4();
    }
    apply {
        if (standard_metadata.ingress_port & 9w0x1 == 9w1) {
            a1_0.apply();
            if (standard_metadata.ingress_port & 9w0x2 == 9w1) {
                _c1.apply();
            }
            if (standard_metadata.ingress_port & 9w0x4 == 9w1) {
                _c2.apply();
            }
        } else {
            b1_0.apply();
            if (standard_metadata.ingress_port & 9w0x2 == 9w1) {
                _c1.apply();
            }
            if (standard_metadata.ingress_port & 9w0x4 == 9w1) {
                _c2.apply();
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
