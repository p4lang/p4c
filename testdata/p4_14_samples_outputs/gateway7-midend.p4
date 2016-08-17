#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
}

struct metadata {
}

struct headers {
    @name("data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_1() {
    }
    action NoAction_2() {
    }
    @name("setb1") action setb1(bit<8> val) {
        hdr.data.b1 = val;
    }
    @name("setb1") action setb1_1(bit<8> val) {
        hdr.data.b1 = val;
    }
    @name("noop") action noop() {
    }
    @name("noop") action noop_1() {
    }
    @name("test1") table test1_0() {
        actions = {
            setb1();
            noop();
            NoAction_1();
        }
        key = {
            hdr.data.f1: exact;
        }
        default_action = NoAction_1();
    }
    @name("test2") table test2_0() {
        actions = {
            setb1_1();
            noop_1();
            NoAction_2();
        }
        key = {
            hdr.data.f2: exact;
        }
        default_action = NoAction_2();
    }
    apply {
        if (hdr.data.b2 > 8w50) 
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
        packet.emit<data_t>(hdr.data);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
