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

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("data") 
    data_t data;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit(hdr.data);
    }
}

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

V1Switch(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
