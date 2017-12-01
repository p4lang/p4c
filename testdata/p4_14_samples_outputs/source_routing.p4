#include <core.p4>
#include <v1model.p4>

header easyroute_head_t {
    bit<64> preamble;
    bit<32> num_valid;
}

header easyroute_port_t {
    bit<8> port;
}

struct metadata {
}

struct headers {
    @name(".easyroute_head") 
    easyroute_head_t easyroute_head;
    @name(".easyroute_port") 
    easyroute_port_t easyroute_port;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_head") state parse_head {
        packet.extract(hdr.easyroute_head);
        transition select(hdr.easyroute_head.num_valid) {
            32w0: accept;
            default: parse_port;
        }
    }
    @name(".parse_port") state parse_port {
        packet.extract(hdr.easyroute_port);
        transition accept;
    }
    @name(".start") state start {
        transition select((packet.lookahead<bit<64>>())[63:0]) {
            64w0: parse_head;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("._drop") action _drop() {
        mark_to_drop();
    }
    @name(".route") action route() {
        standard_metadata.egress_spec = (bit<9>)hdr.easyroute_port.port;
        hdr.easyroute_head.num_valid = hdr.easyroute_head.num_valid + 32w4294967295;
        hdr.easyroute_port.setInvalid();
    }
    @name(".route_pkt") table route_pkt {
        actions = {
            _drop;
            route;
        }
        key = {
            hdr.easyroute_port.isValid(): exact;
        }
        size = 1;
    }
    apply {
        route_pkt.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.easyroute_head);
        packet.emit(hdr.easyroute_port);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

