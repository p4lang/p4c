#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

header data_t {
    bit<128> f1;
    bit<128> f2;
    bit<128> f3;
    bit<128> f4;
    bit<8>   b1;
    bit<8>   b2;
    bit<8>   b3;
    bit<8>   b4;
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
    @name("setb1") action setb1(bit<8> val) {
        hdr.data.b1 = val;
    }
    @name("noop") action noop() {
    }
    @name("test1") table test1() {
        actions = {
            setb1;
            noop;
        }
        key = {
            hdr.data.f1: exact;
            hdr.data.f2: exact;
            hdr.data.f3: exact;
        }
        size = 10000;
    }

    apply {
        test1.apply();
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
