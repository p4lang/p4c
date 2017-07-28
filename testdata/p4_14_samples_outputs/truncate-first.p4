#include <core.p4>
#include <v1model.p4>

header hdrA_t {
    bit<8>  f1;
    bit<64> f2;
}

struct __metadataImpl {
}

struct __headersImpl {
    @name("hdrA") 
    hdrA_t hdrA;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<hdrA_t>(hdr.hdrA);
        transition accept;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name("._nop") action _nop() {
    }
    @name("._truncate") action _truncate(bit<32> new_length, bit<9> port) {
        standard_metadata.egress_spec = port;
        truncate(new_length);
    }
    @name(".t_ingress") table t_ingress {
        actions = {
            _nop();
            _truncate();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdrA.f1: exact @name("hdr.hdrA.f1") ;
        }
        size = 128;
        default_action = NoAction();
    }
    apply {
        t_ingress.apply();
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<hdrA_t>(hdr.hdrA);
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
