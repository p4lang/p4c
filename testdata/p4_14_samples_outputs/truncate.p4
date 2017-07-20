#include <core.p4>
#include <v1model.p4>

header hdrA_t {
    bit<8>  f1;
    bit<64> f2;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("hdrA") 
    hdrA_t hdrA;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.hdrA);
        transition accept;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name("._nop") action _nop() {
    }
    @name("._truncate") action _truncate(bit<32> new_length, bit<9> port) {
        meta.standard_metadata.egress_spec = port;
        truncate((bit<32>)new_length);
    }
    @name(".t_ingress") table t_ingress {
        actions = {
            _nop;
            _truncate;
        }
        key = {
            hdr.hdrA.f1: exact;
        }
        size = 128;
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
        packet.emit(hdr.hdrA);
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
