#include <core.p4>
#include <pna.p4>

header eth_t {
    bit<48> da;
    bit<48> sa;
    bit<16> type;
}

const bit<16> ETYPE_IPV4 = 16w0x800;
header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<6>  dscp;
    bit<2>  ecn;
    bit<16> length;
    bit<16> identification;
    bit<1>  rsvd;
    bit<1>  df;
    bit<1>  mf;
    bit<13> frag_off;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> csum;
    bit<32> src_ip;
    bit<32> dst_ip;
}

const bit<8> IP_PROTO_IPV4 = 8w4;
const bit<8> IP_PROTO_TCP = 8w6;
const bit<8> IP_PROTO_UDP = 8w17;
header udp_t {
    bit<16> sport;
    bit<16> dport;
    bit<16> length;
    bit<16> csum;
}

header tcp_t {
    bit<16> sport;
    bit<16> dport;
    bit<32> seqno;
    bit<32> ackno;
    bit<4>  offset;
    bit<6>  reserved;
    bit<1>  urg;
    bit<1>  ack;
    bit<1>  psh;
    bit<1>  rst;
    bit<1>  syn;
    bit<1>  fin;
    bit<16> window;
    bit<16> csum;
    bit<16> urgptr;
}

struct parsed_headers_t {
    eth_t     mac;
    ipv4_t[4] ipv4;
    udp_t     udp;
    tcp_t     tcp;
}

struct user_meta_t {
    bit<16> L2_packet_len_bytes;
}

control PreControlImpl(in parsed_headers_t hdrs, inout user_meta_t umeta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in pkt, out parsed_headers_t hdrs, inout user_meta_t umeta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<eth_t>(hdrs.mac);
        transition select(hdrs.mac.type) {
            16w0x800: Parse_IPv4_Depth0;
            default: accept;
        }
    }
    state Parse_IPv4_Depth0 {
        pkt.extract<ipv4_t>(hdrs.ipv4[0]);
        transition select(hdrs.ipv4[0].protocol) {
            8w4: Parse_IPv4_Depth1;
            8w17: Parse_UDP;
            8w6: Parse_TCP;
            default: accept;
        }
    }
    state Parse_IPv4_Depth1 {
        pkt.extract<ipv4_t>(hdrs.ipv4[1]);
        transition select(hdrs.ipv4[1].protocol) {
            8w4: Parse_IPv4_Depth2;
            8w17: Parse_UDP;
            8w6: Parse_TCP;
            default: accept;
        }
    }
    state Parse_IPv4_Depth2 {
        pkt.extract<ipv4_t>(hdrs.ipv4[2]);
        transition select(hdrs.ipv4[2].protocol) {
            8w17: Parse_UDP;
            8w6: Parse_TCP;
            default: accept;
        }
    }
    state Parse_TCP {
        pkt.extract<tcp_t>(hdrs.tcp);
        transition accept;
    }
    state Parse_UDP {
        pkt.extract<udp_t>(hdrs.udp);
        transition accept;
    }
}

control MainControlImpl(inout parsed_headers_t hdrs, inout user_meta_t umeta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    action encap_one_tunnel_layer_ipv4(bit<48> mac_da, bit<48> mac_sa, bit<32> ipv4_sa, bit<32> ipv4_da) {
        hdrs.ipv4[3] = hdrs.ipv4[2];
        hdrs.ipv4[2] = hdrs.ipv4[1];
        hdrs.ipv4[1] = hdrs.ipv4[0];
        hdrs.ipv4[0].setInvalid();
        hdrs.mac.da = mac_da;
        hdrs.mac.sa = mac_sa;
        hdrs.mac.type = 16w0x800;
        hdrs.ipv4[0].setValid();
        hdrs.ipv4[0].version = 4w4;
        hdrs.ipv4[0].ihl = 4w5;
        hdrs.ipv4[0].dscp = 6w5;
        hdrs.ipv4[0].ecn = 2w0;
        hdrs.ipv4[0].length = 16w20 + umeta.L2_packet_len_bytes;
        hdrs.ipv4[0].identification = 16w0;
        hdrs.ipv4[0].rsvd = 1w0;
        hdrs.ipv4[0].df = 1w0;
        hdrs.ipv4[0].mf = 1w0;
        hdrs.ipv4[0].frag_off = 13w0;
        hdrs.ipv4[0].ttl = 8w64;
        hdrs.ipv4[0].protocol = 8w4;
        hdrs.ipv4[0].csum = 16w0;
        hdrs.ipv4[0].src_ip = ipv4_sa;
        hdrs.ipv4[0].dst_ip = ipv4_da;
    }
    action decap_one_tunnel_layer_just_before_eth() {
    }
    table header_mod {
        key = {
            hdrs.mac.da: exact @name("hdrs.mac.da");
        }
        actions = {
            @tableonly encap_one_tunnel_layer_ipv4();
            @tableonly decap_one_tunnel_layer_just_before_eth();
            NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        if (hdrs.mac.isValid()) {
            header_mod.apply();
        }
        send_to_port((PortId_t)32w1);
    }
}

control MainDeparserImpl(packet_out pkt, in parsed_headers_t hdrs, in user_meta_t umeta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<eth_t>(hdrs.mac);
        pkt.emit<ipv4_t>(hdrs.ipv4[0]);
        pkt.emit<ipv4_t>(hdrs.ipv4[1]);
        pkt.emit<ipv4_t>(hdrs.ipv4[2]);
        pkt.emit<ipv4_t>(hdrs.ipv4[3]);
        pkt.emit<tcp_t>(hdrs.tcp);
        pkt.emit<udp_t>(hdrs.udp);
    }
}

PNA_NIC<parsed_headers_t, user_meta_t, parsed_headers_t, user_meta_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
