#include <core.p4>
#include <v1model.p4>

header hdr2_t {
    bit<8>  f1;
    bit<8>  f2;
    bit<16> f3;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("hdr2") 
    hdr2_t hdr2;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.hdr2);
        transition accept;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".a21") action a21() {
        meta.standard_metadata.egress_spec = 9w3;
    }
    @name(".a22") action a22() {
        meta.standard_metadata.egress_spec = 9w4;
    }
    @name(".t_ingress_2") table t_ingress_2 {
        actions = {
            a21;
            a22;
        }
        key = {
            hdr.hdr2.f1: exact;
        }
        size = 64;
    }
    apply {
        t_ingress_2.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit(hdr.hdr2);
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
