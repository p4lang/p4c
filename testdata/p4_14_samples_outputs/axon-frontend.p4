#include <core.p4>
#include <v1model.p4>

struct my_metadata_t {
    bit<8>  fwdHopCount;
    bit<8>  revHopCount;
    bit<16> headerLen;
}

header axon_head_t {
    bit<64> preamble;
    bit<8>  axonType;
    bit<16> axonLength;
    bit<8>  fwdHopCount;
    bit<8>  revHopCount;
}

header axon_hop_t {
    bit<8> port;
}

struct metadata {
    @name(".my_metadata") 
    my_metadata_t my_metadata;
}

struct headers {
    @name(".axon_head") 
    axon_head_t    axon_head;
    @name(".axon_fwdHop") 
    axon_hop_t[64] axon_fwdHop;
    @name(".axon_revHop") 
    axon_hop_t[64] axon_revHop;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<64> tmp;
    @name(".parse_fwdHop") state parse_fwdHop {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop.next);
        meta.my_metadata.fwdHopCount = meta.my_metadata.fwdHopCount + 8w255;
        transition parse_next_fwdHop;
    }
    @name(".parse_head") state parse_head {
        packet.extract<axon_head_t>(hdr.axon_head);
        meta.my_metadata.fwdHopCount = hdr.axon_head.fwdHopCount;
        meta.my_metadata.revHopCount = hdr.axon_head.revHopCount;
        meta.my_metadata.headerLen = (bit<16>)(8w2 + hdr.axon_head.fwdHopCount + hdr.axon_head.revHopCount);
        transition select(hdr.axon_head.fwdHopCount) {
            8w0: accept;
            default: parse_next_fwdHop;
        }
    }
    @name(".parse_next_fwdHop") state parse_next_fwdHop {
        transition select(meta.my_metadata.fwdHopCount) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop;
        }
    }
    @name(".parse_next_revHop") state parse_next_revHop {
        transition select(meta.my_metadata.revHopCount) {
            8w0x0: accept;
            default: parse_revHop;
        }
    }
    @name(".parse_revHop") state parse_revHop {
        packet.extract<axon_hop_t>(hdr.axon_revHop.next);
        meta.my_metadata.revHopCount = meta.my_metadata.revHopCount + 8w255;
        transition parse_next_revHop;
    }
    @name(".start") state start {
        tmp = packet.lookahead<bit<64>>();
        transition select(tmp[63:0]) {
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
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("._drop") action _drop() {
        mark_to_drop();
    }
    @name("._drop") action _drop_2() {
        mark_to_drop();
    }
    @name(".route") action route() {
        standard_metadata.egress_spec = (bit<9>)hdr.axon_fwdHop[0].port;
        hdr.axon_head.fwdHopCount = hdr.axon_head.fwdHopCount + 8w255;
        hdr.axon_fwdHop.pop_front(1);
        hdr.axon_head.revHopCount = hdr.axon_head.revHopCount + 8w1;
        hdr.axon_revHop.push_front(1);
        hdr.axon_revHop[0].setValid();
        hdr.axon_revHop[0].port = (bit<8>)standard_metadata.ingress_port;
    }
    @name(".drop_pkt") table drop_pkt_0 {
        actions = {
            _drop();
            @defaultonly NoAction_0();
        }
        size = 1;
        default_action = NoAction_0();
    }
    @name(".route_pkt") table route_pkt_0 {
        actions = {
            _drop_2();
            route();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.axon_head.isValid()     : exact @name("axon_head.$valid$") ;
            hdr.axon_fwdHop[0].isValid(): exact @name("axon_fwdHop[0].$valid$") ;
        }
        size = 1;
        default_action = NoAction_3();
    }
    apply {
        if (hdr.axon_head.axonLength != meta.my_metadata.headerLen) 
            drop_pkt_0.apply();
        else 
            route_pkt_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<axon_head_t>(hdr.axon_head);
        packet.emit<axon_hop_t[64]>(hdr.axon_fwdHop);
        packet.emit<axon_hop_t[64]>(hdr.axon_revHop);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

