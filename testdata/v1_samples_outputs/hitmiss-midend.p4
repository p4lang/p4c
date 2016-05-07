#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

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
    @name("data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    action NoAction_1() {
    }
    action NoAction_2() {
    }
    @name("setb1") action setb1_0(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name("setb1") action setb1(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name("setb1") action setb1_1(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name("noop") action noop_0() {
    }
    @name("noop") action noop() {
    }
    @name("noop") action noop_1() {
    }
    @name("test1") table test1_0() {
        actions = {
            setb1_0;
            noop_0;
            NoAction_0;
        }
        key = {
            hdr.data.f1: ternary;
        }
        default_action = NoAction_0();
    }
    @name("test2") table test2_0() {
        actions = {
            setb1;
            noop;
            NoAction_1;
        }
        key = {
            hdr.data.f2: ternary;
        }
        default_action = NoAction_0();
    }
    @name("test3") table test3_0() {
        actions = {
            setb1_1;
            noop_1;
            NoAction_2;
        }
        key = {
            hdr.data.f3: ternary;
        }
        default_action = NoAction_0();
    }
    apply {
        if (test1_0.apply().hit) 
            test2_0.apply();
        else 
            test3_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.data);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
