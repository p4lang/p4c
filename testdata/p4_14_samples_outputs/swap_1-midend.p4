#include <core.p4>
#include <v1model.p4>

header hdr1_t {
    bit<8> f1;
    bit<8> f2;
}

struct __metadataImpl {
}

struct __headersImpl {
    @name("hdr1") 
    hdr1_t hdr1;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<hdr1_t>(hdr.hdr1);
        transition accept;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name(".a11") action a11_0() {
        standard_metadata.egress_spec = 9w1;
    }
    @name(".a12") action a12_0() {
        standard_metadata.egress_spec = 9w2;
    }
    @name(".t_ingress_1") table t_ingress_0 {
        actions = {
            a11_0();
            a12_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.hdr1.f1: exact @name("hdr.hdr1.f1") ;
        }
        size = 128;
        default_action = NoAction_0();
    }
    apply {
        t_ingress_0.apply();
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

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), egress(), __computeChecksumImpl(), __DeparserImpl()) main;
