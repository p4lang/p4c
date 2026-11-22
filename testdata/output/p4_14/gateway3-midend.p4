#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
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
    @name("._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name(".setb1") action setb1(@name("val") bit<8> val, @name("port") bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name(".setb1") action setb1_1(@name("val") bit<8> val_1, @name("port") bit<9> port_1) {
        hdr.data.b1 = val_1;
        standard_metadata.egress_spec = port_1;
    }
    @name(".noop") action noop() {
    }
    @name(".noop") action noop_1() {
    }
    @name(".set_default_behavior_drop") table set_default_behavior_drop_0 {
        actions = {
            _drop();
        }
        default_action = _drop();
    }
    @name(".test1") table test1_0 {
        actions = {
            setb1();
            noop();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.data.f1: exact @name("data.f1");
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
            hdr.data.f2: exact @name("data.f2");
        }
        default_action = NoAction_2();
    }
    apply {
        set_default_behavior_drop_0.apply();
        if (hdr.data.b2 == hdr.data.b3 || hdr.data.b4 == 8w10) {
            if (hdr.data.b1 == hdr.data.b2 && hdr.data.b4 == 8w10) {
                test1_0.apply();
            }
        } else if (hdr.data.b1 != hdr.data.b2) {
            test2_0.apply();
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
