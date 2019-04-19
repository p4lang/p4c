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
    @name(".data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".noop") action noop() {
    }
    @name(".setb1") action setb1(bit<8> val) {
        hdr.data.b1 = val;
    }
    @name(".setb2") action setb2(bit<8> val) {
        hdr.data.b2 = val;
    }
    @name(".setb3") action setb3(bit<8> val) {
        hdr.data.b3 = val;
    }
    @name(".setb4") action setb4(bit<8> val) {
        hdr.data.b4 = val;
    }
    @name(".test1") table test1 {
        actions = {
            noop;
            setb1;
            setb2;
            setb3;
            setb4;
        }
        key = {
            hdr.data.f1: exact;
        }
    }
    apply {
        if (hdr.data.f2 != 32w0) {
            test1.apply();
        }
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

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

