#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct ingress_metadata_t {
    bit<1>    drop;
    bit<8>    egress_port;
    bit<1024> f1;
    bit<512>  f2;
    bit<256>  f3;
    bit<128>  f4;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

header vag_t {
    bit<1024> f1;
    bit<512>  f2;
    bit<256>  f3;
    bit<128>  f4;
}

struct metadata {
    bit<1>    _ing_metadata_drop0;
    bit<8>    _ing_metadata_egress_port1;
    bit<1024> _ing_metadata_f12;
    bit<512>  _ing_metadata_f23;
    bit<256>  _ing_metadata_f34;
    bit<128>  _ing_metadata_f45;
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
    @name(".set_f1") action set_f1(@name("f1") bit<1024> f1_1) {
        meta._ing_metadata_f12 = f1_1;
    }
    @name(".set_f2") action set_f2(@name("f2") bit<512> f2_1) {
        meta._ing_metadata_f23 = f2_1;
    }
    @name(".set_f3") action set_f3(@name("f3") bit<256> f3_1) {
        meta._ing_metadata_f34 = f3_1;
    }
    @name(".set_f4") action set_f4(@name("f4") bit<128> f4_1) {
        meta._ing_metadata_f45 = f4_1;
    }
    @name(".i_t1") table i_t1_0 {
        actions = {
            nop_2();
            set_f1();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.vag.f1: exact @name("vag.f1");
        }
        default_action = NoAction_3();
    }
    @name(".i_t2") table i_t2_0 {
        actions = {
            nop_3();
            set_f2();
            @defaultonly NoAction_4();
        }
        key = {
            hdr.vag.f2: exact @name("vag.f2");
        }
        default_action = NoAction_4();
    }
    @name(".i_t3") table i_t3_0 {
        actions = {
            nop_4();
            set_f3();
            @defaultonly NoAction_5();
        }
        key = {
            hdr.vag.f3: exact @name("vag.f3");
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
            hdr.vag.f4: ternary @name("vag.f4");
        }
        default_action = NoAction_6();
    }
    apply {
        i_t1_0.apply();
        i_t2_0.apply();
        i_t3_0.apply();
        i_t4_0.apply();
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
