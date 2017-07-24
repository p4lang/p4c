#include <core.p4>
#include <v1model.p4>

header egress {
    bit<32> counterrevolution;
    bit<8>  cartographers;
    bit<8>  wadded;
    bit<32> terns;
    bit<8>  miscellaneous;
}

header computeChecksum {
    bit<32> counterrevolution;
    bit<8>  cartographers;
    bit<8>  wadded;
    bit<32> terns;
    bit<8>  miscellaneous;
}

header headers {
    bit<32> counterrevolution;
    bit<8>  cartographers;
    bit<8>  wadded;
    bit<32> terns;
    bit<8>  miscellaneous;
}

header metadata {
    bit<32> counterrevolution;
    bit<8>  cartographers;
    bit<8>  wadded;
    bit<32> terns;
    bit<8>  miscellaneous;
}

header verifyChecksum {
    bit<32> counterrevolution;
    bit<8>  cartographers;
    bit<8>  wadded;
    bit<32> terns;
    bit<8>  miscellaneous;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("heartlands") 
    egress          heartlands;
    @name("heartlands_c") 
    computeChecksum heartlands_c;
    @name("heartlands_h") 
    headers         heartlands_h;
    @name("heartlands_m") 
    metadata        heartlands_m;
    @name("heartlands_v") 
    verifyChecksum  heartlands_v;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
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
