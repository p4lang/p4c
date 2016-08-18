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
    @name("ing_metadata") 
    ingress_metadata_t ing_metadata;
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("vag") 
    vag_t      vag;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        packet.extract<vag_t>(hdr.vag);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_2() {
    }
    @name("nop") action nop() {
    }
    @name("e_t1") table e_t1_0() {
        actions = {
            nop();
            NoAction_2();
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        default_action = NoAction_2();
    }
    apply {
        e_t1_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_3() {
    }
    action NoAction_4() {
    }
    action NoAction_5() {
    }
    action NoAction_6() {
    }
    @name("nop") action nop_2() {
    }
    @name("nop") action nop_3() {
    }
    @name("nop") action nop_4() {
    }
    @name("nop") action nop_5() {
    }
    @name("ing_drop") action ing_drop() {
        meta.ing_metadata.drop = 1w1;
    }
    @name("set_egress_port") action set_egress_port(bit<8> egress_port) {
        meta.ing_metadata.egress_port = egress_port;
    }
    @name("set_f1") action set_f1(bit<8> f1) {
        meta.ing_metadata.f1 = f1;
    }
    @name("set_f2") action set_f2(bit<16> f2) {
        meta.ing_metadata.f2 = f2;
    }
    @name("set_f3") action set_f3(bit<32> f3) {
        meta.ing_metadata.f3 = f3;
    }
    @name("set_f4") action set_f4(bit<64> f4) {
        meta.ing_metadata.f4 = f4;
    }
    @name("i_t1") table i_t1_0() {
        actions = {
            nop_2();
            ing_drop();
            set_egress_port();
            set_f1();
            NoAction_3();
        }
        key = {
            hdr.vag.f1: exact;
        }
        size = 1024;
        default_action = NoAction_3();
    }
    @name("i_t2") table i_t2_0() {
        actions = {
            nop_3();
            set_f2();
            NoAction_4();
        }
        key = {
            hdr.vag.f2: exact;
        }
        size = 1024;
        default_action = NoAction_4();
    }
    @name("i_t3") table i_t3_0() {
        actions = {
            nop_4();
            set_f3();
            NoAction_5();
        }
        key = {
            hdr.vag.f3: exact;
        }
        size = 1024;
        default_action = NoAction_5();
    }
    @name("i_t4") table i_t4_0() {
        actions = {
            nop_5();
            set_f4();
            NoAction_6();
        }
        key = {
            hdr.vag.f4: exact;
        }
        size = 1024;
        default_action = NoAction_6();
    }
    apply {
        i_t1_0.apply();
        if (meta.ing_metadata.f1 == hdr.vag.f1) {
            i_t2_0.apply();
            if (meta.ing_metadata.f2 == hdr.vag.f2) 
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

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
