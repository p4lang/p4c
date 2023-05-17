#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<8>  b1;
    bit<8>  b2;
    bit<16> h1;
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
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name(".setb1") action setb1(@name("val") bit<8> val, @name("port") bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name(".setb1") action setb1_1(@name("val") bit<8> val_1, @name("port") bit<9> port_1) {
        hdr.data.b1 = val_1;
        standard_metadata.egress_spec = port_1;
    }
    @name(".setb1") action setb1_2(@name("val") bit<8> val_2, @name("port") bit<9> port_2) {
        hdr.data.b1 = val_2;
        standard_metadata.egress_spec = port_2;
    }
    @name(".noop") action noop() {
    }
    @name(".noop") action noop_1() {
    }
    @name(".noop") action noop_2() {
    }
    @name(".test1") table test1_0 {
        actions = {
            setb1();
            noop();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.data.f1: ternary @name("data.f1");
        }
        default_action = NoAction_1();
    }
    @name(".test2") table test2_0 {
        actions = {
            setb1_1();
            noop_1();
            @defaultonly NoAction_2();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2");
        }
        default_action = NoAction_2();
    }
    @name(".test3") table test3_0 {
        actions = {
            setb1_2();
            noop_2();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.data.f3: ternary @name("data.f3");
        }
        default_action = NoAction_3();
    }
    apply {
        if (test1_0.apply().hit) {
            test2_0.apply();
        } else {
            test3_0.apply();
        }
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
