#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
}

header data2_t {
    bit<8> x1;
    bit<8> x2;
}

struct metadata {
}

struct headers {
    @name(".data") 
    data_t  data;
    @name(".data2") 
    data2_t data2;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_data2") state parse_data2 {
        packet.extract<data2_t>(hdr.data2);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition select(hdr.data.b1) {
            8w0x1: parse_data2;
            default: accept;
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("._drop") action _drop() {
        mark_to_drop();
    }
    @name(".setb1") action setb1(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name(".setb1") action setb1_2(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop() {
    }
    @name(".noop") action noop_2() {
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
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f1: exact @name("data.f1") ;
        }
        default_action = NoAction_0();
    }
    @name(".test2") table test2_0 {
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
        set_default_behavior_drop_0.apply();
        if (hdr.data2.isValid()) 
            test1_0.apply();
        test2_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
        packet.emit<data2_t>(hdr.data2);
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

