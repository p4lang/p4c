#include <core.p4>
#include <v1model.p4>

struct metadata_t {
    bit<32> m0;
    bit<32> m1;
    bit<32> m2;
    bit<32> m3;
    bit<32> m4;
    bit<16> m5;
    bit<16> m6;
    bit<16> m7;
    bit<16> m8;
    bit<16> m9;
    bit<8>  m10;
    bit<8>  m11;
    bit<8>  m12;
    bit<8>  m13;
    bit<8>  m14;
}

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
    @name("m") 
    metadata_t          m;
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
    @name(".setmeta") action setmeta(bit<8> v0, bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<16> v5, bit<16> v6) {
        meta.m.m10 = v0;
        meta.m.m1 = v1;
        meta.m.m2 = v2;
        meta.m.m3 = v3;
        meta.m.m4 = v4;
        meta.m.m5 = v5;
        meta.m.m6 = v6;
    }
    @name(".test1") table test1 {
        actions = {
            setmeta;
        }
        key = {
            hdr.data.f1: exact;
        }
        size = 8192;
    }
    apply {
        test1.apply();
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
