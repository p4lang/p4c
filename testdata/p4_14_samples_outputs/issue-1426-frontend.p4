#include <core.p4>
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
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_5() {
    }
    @name(".NoAction") action NoAction_6() {
    }
    @name(".NoAction") action NoAction_7() {
    }
    @name(".send") action send(bit<9> port) {
        standard_metadata.egress_port = port;
    }
    @name(".send") action send_2(bit<9> port) {
        standard_metadata.egress_port = port;
    }
    @name(".discard") action discard() {
        mark_to_drop();
    }
    @name(".discard") action discard_2() {
        mark_to_drop();
    }
    @name(".a1") table a1_0 {
        actions = {
            send();
            discard();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr") ;
        }
        size = 1024;
        default_action = NoAction_0();
    }
    @name(".b1") table b1_0 {
        actions = {
            send_2();
            discard_2();
            @defaultonly NoAction_5();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr") ;
        }
        size = 1024;
        default_action = NoAction_5();
    }
    @name(".send") action _send_0(bit<9> port) {
        standard_metadata.egress_port = port;
    }
    @name(".send") action _send_2(bit<9> port) {
        standard_metadata.egress_port = port;
    }
    @name(".discard") action _discard_0() {
        mark_to_drop();
    }
    @name(".discard") action _discard_2() {
        mark_to_drop();
    }
    @name(".c1") table _c1 {
        actions = {
            _send_0();
            _discard_0();
            @defaultonly NoAction_6();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr") ;
        }
        size = 1024;
        default_action = NoAction_6();
    }
    @name(".c2") table _c2 {
        actions = {
            _send_2();
            _discard_2();
            @defaultonly NoAction_7();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr") ;
        }
        size = 1024;
        default_action = NoAction_7();
    }
    apply {
        if (standard_metadata.ingress_port & 9w0x1 == 9w1) {
            a1_0.apply();
            if (standard_metadata.ingress_port & 9w0x2 == 9w1) 
                _c1.apply();
            if (standard_metadata.ingress_port & 9w0x4 == 9w1) 
                _c2.apply();
        }
        else {
            b1_0.apply();
            if (standard_metadata.ingress_port & 9w0x2 == 9w1) 
                _c1.apply();
            if (standard_metadata.ingress_port & 9w0x4 == 9w1) 
                _c2.apply();
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

