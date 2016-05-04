#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

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
        packet.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    @name("do_add") action do_add_0() {
        hdr.data.b3 = hdr.data.b1 + hdr.data.b2;
    }
    @name("do_and") action do_and_0() {
        hdr.data.b2 = hdr.data.b3 & hdr.data.b4;
    }
    @name("do_or") action do_or_0() {
        hdr.data.b4 = hdr.data.b3 | hdr.data.b1;
    }
    @name("do_xor") action do_xor_0() {
        hdr.data.b1 = hdr.data.b2 ^ hdr.data.b3;
    }
    @name("test1") table test1_0() {
        actions = {
            do_add_0;
            do_and_0;
            do_or_0;
            do_xor_0;
            NoAction_0;
        }
        key = {
            hdr.data.f1: exact;
        }
        default_action = NoAction_0();
    }
    apply {
        test1_0.apply();
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
