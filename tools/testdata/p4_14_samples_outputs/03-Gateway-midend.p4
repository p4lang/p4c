#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<1>  drop;
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

header vag_t {
    bit<8>  f1;
    bit<16> f2;
    bit<32> f3;
    bit<64> f4;
}

struct metadata {
    bit<1>  _ing_metadata_drop0;
    bit<8>  _ing_metadata_egress_port1;
    bit<8>  _ing_metadata_f12;
    bit<16> _ing_metadata_f23;
    bit<32> _ing_metadata_f34;
    bit<64> _ing_metadata_f45;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
    @name(".vag") 
    vag_t      vag;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        packet.extract<vag_t>(hdr.vag);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".nop") action nop() {
    }
    @name(".e_t1") table e_t1_0 {
        actions = {
            nop();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("ethernet.srcAddr") ;
        }
        default_action = NoAction_0();
    }
    apply {
        e_t1_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_1() {
    }
    @name(".NoAction") action NoAction_7() {
    }
    @name(".NoAction") action NoAction_8() {
    }
    @name(".NoAction") action NoAction_9() {
    }
    @name(".nop") action nop_2() {
    }
    @name(".nop") action nop_6() {
    }
    @name(".nop") action nop_7() {
    }
    @name(".nop") action nop_8() {
    }
    @name(".ing_drop") action ing_drop() {
        meta._ing_metadata_drop0 = 1w1;
    }
    @name(".set_egress_port") action set_egress_port(bit<8> egress_port) {
        meta._ing_metadata_egress_port1 = egress_port;
    }
    @name(".set_f1") action set_f1(bit<8> f1) {
        meta._ing_metadata_f12 = f1;
    }
    @name(".set_f2") action set_f2(bit<16> f2) {
        meta._ing_metadata_f23 = f2;
    }
    @name(".set_f3") action set_f3(bit<32> f3) {
        meta._ing_metadata_f34 = f3;
    }
    @name(".set_f4") action set_f4(bit<64> f4) {
        meta._ing_metadata_f45 = f4;
    }
    @name(".i_t1") table i_t1_0 {
        actions = {
            nop_2();
            ing_drop();
            set_egress_port();
            set_f1();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.vag.f1: exact @name("vag.f1") ;
        }
        size = 1024;
        default_action = NoAction_1();
    }
    @name(".i_t2") table i_t2_0 {
        actions = {
            nop_6();
            set_f2();
            @defaultonly NoAction_7();
        }
        key = {
            hdr.vag.f2: exact @name("vag.f2") ;
        }
        size = 1024;
        default_action = NoAction_7();
    }
    @name(".i_t3") table i_t3_0 {
        actions = {
            nop_7();
            set_f3();
            @defaultonly NoAction_8();
        }
        key = {
            hdr.vag.f3: exact @name("vag.f3") ;
        }
        size = 1024;
        default_action = NoAction_8();
    }
    @name(".i_t4") table i_t4_0 {
        actions = {
            nop_8();
            set_f4();
            @defaultonly NoAction_9();
        }
        key = {
            hdr.vag.f4: exact @name("vag.f4") ;
        }
        size = 1024;
        default_action = NoAction_9();
    }
    apply {
        i_t1_0.apply();
        if (meta._ing_metadata_f12 == hdr.vag.f1) {
            i_t2_0.apply();
            if (meta._ing_metadata_f23 == hdr.vag.f2) 
                i_t3_0.apply();
        }
        else 
            i_t4_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<vag_t>(hdr.vag);
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

