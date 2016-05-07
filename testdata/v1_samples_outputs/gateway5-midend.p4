#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<2>  x1;
    bit<3>  pad0;
    bit<2>  x2;
    bit<5>  pad1;
    bit<1>  x3;
    bit<2>  pad2;
    bit<32> skip;
    bit<1>  x4;
    bit<1>  x5;
    bit<6>  pad3;
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
    @name("setf4") action setf4_0(bit<32> val) {
        hdr.data.f4 = val;
    }
    @name("setf4") action setf4(bit<32> val) {
        hdr.data.f4 = val;
    }
    @name("noop") action noop_0() {
    }
    @name("noop") action noop() {
    }
    @name("test1") table test1_0() {
        actions = {
            setf4_0;
            noop_0;
            NoAction_0;
        }
        key = {
            hdr.data.f1: exact;
        }
        default_action = NoAction_0();
    }
    @name("test2") table test2_0() {
        actions = {
            setf4;
            noop;
            NoAction_1;
        }
        key = {
            hdr.data.f2: exact;
        }
        default_action = NoAction_0();
    }
    apply {
        if (hdr.data.x1 == 2w1 && hdr.data.x4 == 1w0) 
            test1_0.apply();
        else 
            test2_0.apply();
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
