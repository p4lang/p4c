#include <core.p4>
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
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_1() {
    }
    @name(".NoAction") action NoAction_9() {
    }
    @name(".setf1") action setf1(bit<32> val) {
        hdr.data.f1 = val;
    }
    @name(".noop") action noop() {
    }
    @name(".noop") action noop_2() {
    }
    @name(".noop") action noop_8() {
    }
    @name(".setb1") action setb1(bit<32> val) {
        hdr.data.b1 = val;
    }
    @name(".setb2") action setb2(bit<32> val) {
        hdr.data.b2 = val;
    }
    @name(".E1") table E1_0 {
        actions = {
            setf1();
            noop();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2") ;
        }
        default_action = NoAction_0();
    }
    @name(".EA") table EA_0 {
        actions = {
            setb1();
            noop_2();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.data.f3: ternary @name("data.f3") ;
        }
        default_action = NoAction_1();
    }
    @name(".EB") table EB_0 {
        actions = {
            setb2();
            noop_8();
            @defaultonly NoAction_9();
        }
        key = {
            hdr.data.f3: ternary @name("data.f3") ;
        }
        default_action = NoAction_9();
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
    @name(".NoAction") action NoAction_10() {
    }
    @name(".NoAction") action NoAction_11() {
    }
    @name(".NoAction") action NoAction_12() {
    }
    @name(".NoAction") action NoAction_13() {
    }
    @name(".setb1") action setb1_2(bit<32> val) {
        hdr.data.b1 = val;
    }
    @name(".noop") action noop_9() {
    }
    @name(".noop") action noop_10() {
    }
    @name(".noop") action noop_11() {
    }
    @name(".noop") action noop_12() {
    }
    @name(".setb3") action setb3(bit<32> val) {
        hdr.data.b3 = val;
    }
    @name(".setb2") action setb2_2(bit<32> val) {
        hdr.data.b2 = val;
    }
    @name(".setb4") action setb4(bit<32> val) {
        hdr.data.b4 = val;
    }
    @name(".A1") table A1_0 {
        actions = {
            setb1_2();
            noop_9();
            @defaultonly NoAction_10();
        }
        key = {
            hdr.data.f1: ternary @name("data.f1") ;
        }
        default_action = NoAction_10();
    }
    @name(".A2") table A2_0 {
        actions = {
            setb3();
            noop_10();
            @defaultonly NoAction_11();
        }
        key = {
            hdr.data.b1: ternary @name("data.b1") ;
        }
        default_action = NoAction_11();
    }
    @name(".B1") table B1_0 {
        actions = {
            setb2_2();
            noop_11();
            @defaultonly NoAction_12();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2") ;
        }
        default_action = NoAction_12();
    }
    @name(".B2") table B2_0 {
        actions = {
            setb4();
            noop_12();
            @defaultonly NoAction_13();
        }
        key = {
            hdr.data.b2: ternary @name("data.b2") ;
        }
        default_action = NoAction_13();
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

