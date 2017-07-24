#include <core.p4>
#include <v1model.p4>

header hdr2_t {
    bit<8>  f1;
    bit<8>  f2;
    bit<16> f3;
}

struct __metadataImpl {
}

struct __headersImpl {
    @name("hdr2") 
    hdr2_t hdr2;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<hdr2_t>(hdr.hdr2);
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
    @name(".a21") action a21_0() {
        standard_metadata.egress_spec = 9w3;
    }
    @name(".a22") action a22_0() {
        standard_metadata.egress_spec = 9w4;
    }
    @name(".t_ingress_2") table t_ingress_0 {
        actions = {
            a21_0();
            a22_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.hdr2.f1: exact @name("hdr.hdr2.f1") ;
        }
        size = 64;
        default_action = NoAction_0();
    }
    apply {
        t_ingress_0.apply();
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<hdr2_t>(hdr.hdr2);
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
