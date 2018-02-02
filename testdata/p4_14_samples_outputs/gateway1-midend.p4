#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<8>  b1;
    bit<8>  b2;
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
    @name(".NoAction") action NoAction_3() {
    }
    @name("._drop") action _drop_0() {
        mark_to_drop();
    }
    @name(".setb1") action setb1_0(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name(".setb1") action setb1_2(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop_0() {
    }
    @name(".noop") action noop_2() {
    }
    @name(".set_default_behavior_drop") table set_default_behavior_drop {
        actions = {
            _drop_0();
        }
        default_action = _drop_0();
    }
    @name(".test1") table test1 {
        actions = {
            setb1_0();
            noop_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f1: exact @name("data.f1") ;
        }
        default_action = NoAction_0();
    }
    @name(".test2") table test2 {
        actions = {
            setb1_2();
            noop_2();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.data.f2: exact @name("data.f2") ;
        }
        default_action = NoAction_3();
    }
    apply {
        set_default_behavior_drop.apply();
        if (hdr.data.b2 == 8w1) 
            test1.apply();
        else 
            test2.apply();
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

