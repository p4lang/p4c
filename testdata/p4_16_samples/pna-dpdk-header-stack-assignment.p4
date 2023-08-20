/* Issue #4087 p4c-dpdk back end gives error for assignment between elements of a header stack */
#include <core.p4>
#include <pna.p4>

header eth_t {
    bit<48> da;
    bit<48> sa;
    bit<16> type;
}

const bit<16> ETYPE_IPV4 = 0x800;

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

const bit<8> IP_PROTO_IPV4 = 4;
const bit<8> IP_PROTO_TCP = 6;
const bit<8> IP_PROTO_UDP = 17;

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
    eth_t         mac;
    ipv4_t[3 + 1] ipv4;
    udp_t         udp;
    tcp_t         tcp;
}

struct user_meta_t {
    bit<16> L2_packet_len_bytes;
}

control PreControlImpl(
    in parsed_headers_t hdrs,
    inout user_meta_t umeta,
    in pna_pre_input_metadata_t istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {
    }
}

parser MainParserImpl(
    packet_in pkt,
    out parsed_headers_t hdrs,
    inout user_meta_t umeta,
    in pna_main_parser_input_metadata_t istd)
{
    state start {
        pkt.extract(hdrs.mac);
        transition select(hdrs.mac.type) {
            ETYPE_IPV4: Parse_IPv4_Depth0;
            default: accept;
        }
    }
    state Parse_IPv4_Depth0 {
        pkt.extract(hdrs.ipv4[0]);
        transition select(hdrs.ipv4[0].protocol) {
            IP_PROTO_IPV4: Parse_IPv4_Depth1;
            IP_PROTO_UDP: Parse_UDP;
            IP_PROTO_TCP: Parse_TCP;
            default: accept;
        }
    }
    state Parse_IPv4_Depth1 {
        pkt.extract(hdrs.ipv4[1]);
        transition select(hdrs.ipv4[1].protocol) {
            IP_PROTO_IPV4: Parse_IPv4_Depth2;
            IP_PROTO_UDP: Parse_UDP;
            IP_PROTO_TCP: Parse_TCP;
            default: accept;
        }
    }
    state Parse_IPv4_Depth2 {
        pkt.extract(hdrs.ipv4[2]);
        transition select(hdrs.ipv4[2].protocol) {
            IP_PROTO_UDP: Parse_UDP;
            IP_PROTO_TCP: Parse_TCP;
            default: accept;
        }
    }
    state Parse_TCP {
        pkt.extract(hdrs.tcp);
        transition accept;
    }
    state Parse_UDP {
        pkt.extract(hdrs.udp);
        transition accept;
    }
}

control MainControlImpl(
    inout parsed_headers_t hdrs,
    inout user_meta_t umeta,
    in pna_main_input_metadata_t istd,
    inout pna_main_output_metadata_t ostd)
{
    action encap_one_tunnel_layer_ipv4(
        bit<48> mac_da,
        bit<48> mac_sa,
        bit<32> ipv4_sa,
        bit<32> ipv4_da)
    {
        hdrs.ipv4[3] = hdrs.ipv4[2];
        hdrs.ipv4[2] = hdrs.ipv4[1];
        hdrs.ipv4[1] = hdrs.ipv4[0];
        hdrs.ipv4[0].setInvalid();
        hdrs.mac.da = mac_da;
        hdrs.mac.sa = mac_sa;
        hdrs.mac.type = ETYPE_IPV4;
        hdrs.ipv4[0].setValid();
        hdrs.ipv4[0].version = 4;
        hdrs.ipv4[0].ihl = 5;
        hdrs.ipv4[0].dscp = 5;
        hdrs.ipv4[0].ecn = 0;
        hdrs.ipv4[0].length = (hdrs.ipv4[0].maxSizeInBytes() +
            umeta.L2_packet_len_bytes);
        hdrs.ipv4[0].identification = 0;
        hdrs.ipv4[0].rsvd = 0;
        hdrs.ipv4[0].df = 0;
        hdrs.ipv4[0].mf = 0;
        hdrs.ipv4[0].frag_off = 0;
        hdrs.ipv4[0].ttl = 64;
        hdrs.ipv4[0].protocol = IP_PROTO_IPV4;
        hdrs.ipv4[0].csum = 0;
        hdrs.ipv4[0].src_ip = ipv4_sa;
        hdrs.ipv4[0].dst_ip = ipv4_da;
    }
    action decap_one_tunnel_layer_just_before_eth() {
        // Uncomment below statement when  pop_front is supported.
        // hdrs.ipv4.pop_front(1);
    }
    table header_mod {
        key = {
            hdrs.mac.da: exact;
        }
        actions = {
            @tableonly encap_one_tunnel_layer_ipv4;
            @tableonly decap_one_tunnel_layer_just_before_eth;
            NoAction;
        }
        const default_action = NoAction;
    }
    apply {
        if (hdrs.mac.isValid()) {
            header_mod.apply();
        }
        send_to_port((PortId_t)1);
    }
}

control MainDeparserImpl(
    packet_out pkt,
    in parsed_headers_t hdrs,
    in user_meta_t umeta,
    in pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdrs.mac);
        pkt.emit(hdrs.ipv4[0]);
        pkt.emit(hdrs.ipv4[1]);
        pkt.emit(hdrs.ipv4[2]);
        pkt.emit(hdrs.ipv4[3]);
        pkt.emit(hdrs.tcp);
        pkt.emit(hdrs.udp);
    }
}

PNA_NIC(
    MainParserImpl(),
    PreControlImpl(),
    MainControlImpl(),
    MainDeparserImpl()) main;

