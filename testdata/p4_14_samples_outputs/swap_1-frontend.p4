#include <core.p4>
#include <v1model.p4>

header hdr1_t {
    bit<8> f1;
    bit<8> f2;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("hdr1") 
    hdr1_t hdr1;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract<hdr1_t>(hdr.hdr1);
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".a11") action a11_0() {
        meta.standard_metadata.egress_spec = 9w1;
    }
    @name(".a12") action a12_0() {
        meta.standard_metadata.egress_spec = 9w2;
    }
    @name(".t_ingress_1") table t_ingress {
        actions = {
            a11_0();
            a12_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdr1.f1: exact @name("hdr.hdr1.f1") ;
        }
        size = 128;
        default_action = NoAction();
    }
    apply {
        t_ingress.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<hdr1_t>(hdr.hdr1);
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

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
