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
    bit<8>  _my_metadata_fwdHopCount0;
    bit<8>  _my_metadata_revHopCount1;
    bit<16> _my_metadata_headerLen2;
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
        meta._my_metadata_fwdHopCount0 = meta._my_metadata_fwdHopCount0 + 8w255;
        transition parse_next_fwdHop;
    }
    @name(".parse_head") state parse_head {
        packet.extract<axon_head_t>(hdr.axon_head);
        meta._my_metadata_fwdHopCount0 = hdr.axon_head.fwdHopCount;
        meta._my_metadata_revHopCount1 = hdr.axon_head.revHopCount;
        meta._my_metadata_headerLen2 = (bit<16>)(8w2 + hdr.axon_head.fwdHopCount + hdr.axon_head.revHopCount);
        transition select(hdr.axon_head.fwdHopCount) {
            8w0: accept;
            default: parse_next_fwdHop;
        }
    }
    @name(".parse_next_fwdHop") state parse_next_fwdHop {
        transition select(meta._my_metadata_fwdHopCount0) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop;
        }
    }
    @name(".parse_next_revHop") state parse_next_revHop {
        transition select(meta._my_metadata_revHopCount1) {
            8w0x0: accept;
            default: parse_revHop;
        }
    }
    @name(".parse_revHop") state parse_revHop {
        packet.extract<axon_hop_t>(hdr.axon_revHop.next);
        meta._my_metadata_revHopCount1 = meta._my_metadata_revHopCount1 + 8w255;
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
        if (hdr.axon_head.axonLength != meta._my_metadata_headerLen2) 
            drop_pkt_0.apply();
        else 
            route_pkt_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<axon_head_t>(hdr.axon_head);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[0]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[1]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[2]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[3]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[4]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[5]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[6]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[7]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[8]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[9]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[10]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[11]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[12]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[13]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[14]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[15]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[16]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[17]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[18]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[19]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[20]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[21]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[22]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[23]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[24]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[25]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[26]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[27]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[28]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[29]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[30]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[31]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[32]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[33]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[34]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[35]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[36]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[37]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[38]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[39]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[40]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[41]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[42]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[43]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[44]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[45]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[46]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[47]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[48]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[49]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[50]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[51]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[52]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[53]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[54]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[55]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[56]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[57]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[58]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[59]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[60]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[61]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[62]);
        packet.emit<axon_hop_t>(hdr.axon_fwdHop[63]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[0]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[1]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[2]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[3]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[4]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[5]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[6]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[7]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[8]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[9]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[10]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[11]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[12]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[13]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[14]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[15]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[16]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[17]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[18]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[19]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[20]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[21]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[22]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[23]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[24]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[25]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[26]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[27]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[28]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[29]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[30]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[31]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[32]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[33]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[34]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[35]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[36]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[37]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[38]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[39]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[40]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[41]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[42]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[43]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[44]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[45]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[46]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[47]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[48]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[49]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[50]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[51]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[52]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[53]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[54]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[55]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[56]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[57]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[58]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[59]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[60]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[61]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[62]);
        packet.emit<axon_hop_t>(hdr.axon_revHop[63]);
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

