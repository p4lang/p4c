#include <core.p4>
#include <v1model.p4>

header easyroute_head_t {
    bit<64> preamble;
    bit<32> num_valid;
}

header easyroute_port_t {
    bit<8> port;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("easyroute_head") 
    easyroute_head_t easyroute_head;
    @name("easyroute_port") 
    easyroute_port_t easyroute_port;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    bit<64> tmp;
    @name(".parse_head") state parse_head {
        packet.extract<easyroute_head_t>(hdr.easyroute_head);
        transition select(hdr.easyroute_head.num_valid) {
            32w0: accept;
            default: parse_port;
        }
    }
    @name(".parse_port") state parse_port {
        packet.extract<easyroute_port_t>(hdr.easyroute_port);
        transition accept;
    }
    @name(".start") state start {
        tmp = packet.lookahead<bit<64>>();
        transition select(tmp[63:0]) {
            64w0: parse_head;
            default: accept;
        }
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name("._drop") action _drop_0() {
        mark_to_drop();
    }
    @name(".route") action route_0() {
        meta.standard_metadata.egress_spec = (bit<9>)hdr.easyroute_port.port;
        hdr.easyroute_head.num_valid = hdr.easyroute_head.num_valid + 32w4294967295;
        hdr.easyroute_port.setInvalid();
    }
    @name(".route_pkt") table route_pkt_0 {
        actions = {
            _drop_0();
            route_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.easyroute_port.isValid(): exact @name("hdr.easyroute_port.isValid()") ;
        }
        size = 1;
        default_action = NoAction();
    }
    apply {
        route_pkt_0.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<easyroute_head_t>(hdr.easyroute_head);
        packet.emit<easyroute_port_t>(hdr.easyroute_port);
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
