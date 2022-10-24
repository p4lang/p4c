#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<32> b1;
    bit<32> b2;
    bit<32> b3;
    bit<32> b4;
}

struct metadata {
}

struct headers {
    @name(".data")
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @name(".setf1") action setf1(@name("val") bit<32> val) {
        hdr.data.f1 = val;
    }
    @name(".noop") action noop() {
    }
    @name(".noop") action noop_2() {
    }
    @name(".noop") action noop_3() {
    }
    @name(".setb1") action setb1(@name("val") bit<32> val_7) {
        hdr.data.b1 = val_7;
    }
    @name(".setb2") action setb2(@name("val") bit<32> val_8) {
        hdr.data.b2 = val_8;
    }
    @name(".E1") table E1_0 {
        actions = {
            setf1();
            noop();
            @defaultonly NoAction_2();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2");
        }
        default_action = NoAction_2();
    }
    @name(".EA") table EA_0 {
        actions = {
            setb1();
            noop_2();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.data.f3: ternary @name("data.f3");
        }
        default_action = NoAction_3();
    }
    @name(".EB") table EB_0 {
        actions = {
            setb2();
            noop_3();
            @defaultonly NoAction_4();
        }
        key = {
            hdr.data.f3: ternary @name("data.f3");
        }
        default_action = NoAction_4();
    }
    apply {
        E1_0.apply();
        if (hdr.data.f1 == 32w0) {
            EA_0.apply();
        } else {
            EB_0.apply();
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_6() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_7() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_8() {
    }
    @name(".setb1") action setb1_2(@name("val") bit<32> val_9) {
        hdr.data.b1 = val_9;
    }
    @name(".noop") action noop_4() {
    }
    @name(".noop") action noop_5() {
    }
    @name(".noop") action noop_6() {
    }
    @name(".noop") action noop_7() {
    }
    @name(".setb3") action setb3(@name("val") bit<32> val_10) {
        hdr.data.b3 = val_10;
    }
    @name(".setb2") action setb2_2(@name("val") bit<32> val_11) {
        hdr.data.b2 = val_11;
    }
    @name(".setb4") action setb4(@name("val") bit<32> val_12) {
        hdr.data.b4 = val_12;
    }
    @name(".A1") table A1_0 {
        actions = {
            setb1_2();
            noop_4();
            @defaultonly NoAction_5();
        }
        key = {
            hdr.data.f1: ternary @name("data.f1");
        }
        default_action = NoAction_5();
    }
    @name(".A2") table A2_0 {
        actions = {
            setb3();
            noop_5();
            @defaultonly NoAction_6();
        }
        key = {
            hdr.data.b1: ternary @name("data.b1");
        }
        default_action = NoAction_6();
    }
    @name(".B1") table B1_0 {
        actions = {
            setb2_2();
            noop_6();
            @defaultonly NoAction_7();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2");
        }
        default_action = NoAction_7();
    }
    @name(".B2") table B2_0 {
        actions = {
            setb4();
            noop_7();
            @defaultonly NoAction_8();
        }
        key = {
            hdr.data.b2: ternary @name("data.b2");
        }
        default_action = NoAction_8();
    }
    apply {
        if (hdr.data.b1 == 32w0) {
            A1_0.apply();
            A2_0.apply();
        }
        B1_0.apply();
        B2_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
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
