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

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_5() {
    }
    @name(".NoAction") action NoAction_6() {
    }
    @name(".NoAction") action NoAction_7() {
    }
    @name(".setb1") action setb1(bit<32> val) {
        hdr.data.b1 = val;
    }
    @name(".noop") action noop() {
    }
    @name(".noop") action noop_4() {
    }
    @name(".noop") action noop_5() {
    }
    @name(".noop") action noop_6() {
    }
    @name(".setb3") action setb3(bit<32> val) {
        hdr.data.b3 = val;
    }
    @name(".setb2") action setb2(bit<32> val) {
        hdr.data.b2 = val;
    }
    @name(".setb4") action setb4(bit<32> val) {
        hdr.data.b4 = val;
    }
    @name(".A1") table A1_0 {
        actions = {
            setb1();
            noop();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f1: ternary @name("data.f1") ;
        }
        default_action = NoAction_0();
    }
    @name(".A2") table A2_0 {
        actions = {
            setb3();
            noop_4();
            @defaultonly NoAction_5();
        }
        key = {
            hdr.data.b1: ternary @name("data.b1") ;
        }
        default_action = NoAction_5();
    }
    @name(".B1") table B1_0 {
        actions = {
            setb2();
            noop_5();
            @defaultonly NoAction_6();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2") ;
        }
        default_action = NoAction_6();
    }
    @name(".B2") table B2_0 {
        actions = {
            setb4();
            noop_6();
            @defaultonly NoAction_7();
        }
        key = {
            hdr.data.b2: ternary @name("data.b2") ;
        }
        default_action = NoAction_7();
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

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
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

