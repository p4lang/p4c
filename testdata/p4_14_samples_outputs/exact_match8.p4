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
    @name(".setb12") action setb12(bit<8> v1, bit<8> v2) {
        hdr.data.b1 = v1;
        hdr.data.b2 = v2;
    }
    @name(".setb13") action setb13(bit<8> v1, bit<8> v2) {
        hdr.data.b1 = v1;
        hdr.data.b3 = v2;
    }
    @name(".setb14") action setb14(bit<8> v1, bit<8> v2) {
        hdr.data.b1 = v1;
        hdr.data.b4 = v2;
    }
    @name(".setb23") action setb23(bit<8> v1, bit<8> v2) {
        hdr.data.b2 = v1;
        hdr.data.b3 = v2;
    }
    @name(".setb24") action setb24(bit<8> v1, bit<8> v2) {
        hdr.data.b2 = v1;
        hdr.data.b4 = v2;
    }
    @name(".setb34") action setb34(bit<8> v1, bit<8> v2) {
        hdr.data.b3 = v1;
        hdr.data.b4 = v2;
    }
    @name(".test1") table test1 {
        actions = {
            noop;
            setb1;
            setb2;
            setb3;
            setb4;
            setb12;
            setb13;
            setb14;
            setb23;
            setb24;
            setb34;
        }
        key = {
            hdr.data.f1: exact;
        }
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

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

