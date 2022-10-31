#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct meta_t {
    bit<16> x;
    bit<16> y;
    bit<16> z;
}

header hdr0_t {
    bit<16> a;
    bit<8>  b;
    bit<8>  c;
}

struct metadata {
    @name(".meta")
    meta_t meta;
}

struct headers {
    @name(".hdr0")
    hdr0_t hdr0;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".p_hdr0") state p_hdr0 {
        packet.extract<hdr0_t>(hdr.hdr0);
        transition accept;
    }
    @name(".start") state start {
        transition p_hdr0;
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
    @name(".do_nothing") action do_nothing() {
    }
    @name(".do_nothing") action do_nothing_1() {
    }
    @name(".do_nothing") action do_nothing_2() {
    }
    @name(".do_nothing") action do_nothing_3() {
    }
    @name(".action_0") action action_0(@name("p") bit<8> p_3) {
        meta.meta.x = 16w1;
        meta.meta.y = 16w2;
    }
    @name(".action_1") action action_1(@name("p") bit<8> p_4) {
        meta.meta.z = meta.meta.y + meta.meta.x;
    }
    @name(".action_1") action action_2(@name("p") bit<8> p_5) {
        meta.meta.z = meta.meta.y + meta.meta.x;
    }
    @name(".action_2") action action_6(@name("p") bit<8> p_6) {
        hdr.hdr0.a = meta.meta.z;
    }
    @name(".t0") table t0_0 {
        actions = {
            do_nothing();
            action_0();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.hdr0.a: ternary @name("hdr0.a");
        }
        size = 512;
        default_action = NoAction_1();
    }
    @name(".t1") table t1_0 {
        actions = {
            do_nothing_1();
            action_1();
            @defaultonly NoAction_2();
        }
        key = {
            hdr.hdr0.a: ternary @name("hdr0.a");
        }
        size = 512;
        default_action = NoAction_2();
    }
    @name(".t2") table t2_0 {
        actions = {
            do_nothing_2();
            action_6();
            @defaultonly NoAction_3();
        }
        key = {
            meta.meta.y: ternary @name("meta.y");
            meta.meta.z: exact @name("meta.z");
        }
        size = 512;
        default_action = NoAction_3();
    }
    @name(".t3") table t3_0 {
        actions = {
            do_nothing_3();
            action_2();
            @defaultonly NoAction_4();
        }
        key = {
            hdr.hdr0.a: ternary @name("hdr0.a");
        }
        size = 512;
        default_action = NoAction_4();
    }
    apply {
        if (hdr.hdr0.isValid()) {
            t0_0.apply();
        }
        if (hdr.hdr0.isValid()) {
            ;
        } else {
            t1_0.apply();
        }
        if (hdr.hdr0.isValid() || hdr.hdr0.isValid()) {
            t2_0.apply();
        }
        if (hdr.hdr0.isValid()) {
            t3_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<hdr0_t>(hdr.hdr0);
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
