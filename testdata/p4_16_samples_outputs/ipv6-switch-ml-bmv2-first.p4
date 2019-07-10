#include <core.p4>
#include <v1model.p4>

typedef bit<48> mac_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<9> port_t;
const bit<16> TYPE_IPV6 = 16w0x86dd;
const bit<9> TYPE_CPU = 9w192;
const bit<8> PROTO_TCP = 8w6;
const bit<8> PROTO_UDP = 8w17;
const bit<8> PROTO_ICMP6 = 8w58;
const bit<8> TCP_SEQ_LEN = 8w4;
const bit<8> ICMP6_ECHO_REQUEST = 8w128;
const bit<8> ICMP6_ECHO_REPLY = 8w129;
header ethernet_t {
    mac_addr_t dstAddr;
    mac_addr_t srcAddr;
    bit<16>    etherType;
}

header ipv6_t {
    bit<4>      version;
    bit<8>      traffic_class;
    bit<20>     flow_label;
    bit<16>     payload_length;
    bit<8>      nextHdr;
    bit<8>      hopLimit;
    ipv6_addr_t srcAddr;
    ipv6_addr_t dstAddr;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    int<32> seqNo;
    int<32> ackNo;
    bit<4>  data_offset;
    bit<4>  res;
    bit<1>  cwr;
    bit<1>  ece;
    bit<1>  urg;
    bit<1>  ack;
    bit<1>  psh;
    bit<1>  rst;
    bit<1>  syn;
    bit<1>  fin;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

header icmp6_t {
    bit<8>  type;
    bit<8>  code;
    bit<16> checksum;
}

struct headers {
    ethernet_t inner_ethernet;
    icmp6_t    inner_icmp6;
    ipv6_t     inner_ipv6;
    tcp_t      inner_tcp;
    udp_t      inner_udp;
    ethernet_t ethernet;
    icmp6_t    icmp6;
    ipv6_t     ipv6;
    tcp_t      tcp;
    udp_t      udp;
}

struct l3_metadata_t {
    bit<2>  lkp_ip_type;
    bit<4>  lkp_ip_version;
    bit<8>  lkp_ip_proto;
    bit<8>  lkp_dscp;
    bit<8>  lkp_ip_ttl;
    bit<16> lkp_l4_sport;
    bit<16> lkp_l4_dport;
    bit<16> lkp_outer_l4_sport;
    bit<16> lkp_outer_l4_dport;
    bit<16> vrf;
    bit<10> rmac_group;
    bit<1>  rmac_hit;
    bit<2>  urpf_mode;
    bit<1>  urpf_hit;
    bit<1>  urpf_check_fail;
    bit<16> urpf_bd_group;
    bit<1>  fib_hit;
    bit<16> fib_nexthop;
    bit<2>  fib_nexthop_type;
    bit<16> same_bd_check;
    bit<16> nexthop_index;
    bit<1>  routed;
    bit<1>  outer_routed;
    bit<8>  mtu_index;
    bit<1>  l3_copy;
    bit<16> l3_mtu_check;
    bit<16> egress_l4_sport;
    bit<16> egress_l4_dport;
}

struct tunnel_metadata_t {
    bit<5>  ingress_tunnel_type;
    bit<24> tunnel_vni;
    bit<5>  egress_tunnel_type;
    bit<14> tunnel_index;
    bit<9>  tunnel_src_index;
    bit<9>  tunnel_smac_index;
    bit<14> tunnel_dst_index;
    bit<14> tunnel_dmac_index;
    bit<24> vnid;
    bit<1>  tunnel_terminate;
    bit<1>  tunnel_if_check;
    bit<4>  egress_header_count;
    bit<8>  inner_ip_proto;
    bit<1>  skip_encap_inner;
}

struct l2_metadata_t {
    bit<48> lkp_mac_sa;
    bit<48> lkp_mac_da;
    bit<3>  lkp_pkt_type;
    bit<16> lkp_mac_type;
    bit<3>  lkp_pcp;
    bit<16> l2_nexthop;
    bit<2>  l2_nexthop_type;
    bit<1>  l2_redirect;
    bit<1>  l2_src_miss;
    bit<16> l2_src_move;
    bit<10> stp_group;
    bit<3>  stp_state;
    bit<16> bd_stats_idx;
    bit<1>  learning_enabled;
    bit<1>  port_vlan_mapping_miss;
    bit<16> same_if_check;
}

struct ipv6_metadata_t {
    bit<128> lkp_ipv6_sa;
    bit<128> lkp_ipv6_da;
    bit<1>   ipv6_unicast_enabled;
    bit<1>   ipv6_src_is_link_local;
    bit<2>   ipv6_urpf_mode;
}

struct acl_metadata_t {
    bit<1>  acl_deny;
    bit<24> vnid;
    bit<14> acl_stats_index;
    bit<16> egress_if_label;
    bit<16> egress_bd_label;
}

struct fwd_meta_t {
    bit<32> l2ptr;
    bit<24> out_bd;
}

struct metadata_t {
    port_t            ingress_port;
    port_t            egress_port;
    bit<16>           tcp_length;
    bit<32>           cast_length;
    bit<1>            do_cksum;
    l2_metadata_t     l2_metadata;
    l3_metadata_t     l3_metadata;
    ipv6_metadata_t   ipv6_metadata;
    tunnel_metadata_t tunnel_metadata;
    fwd_meta_t        fwd;
}

const bit<32> BMV2_V1MODEL_INSTANCE_TYPE_REPLICATION = 32w5;
parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x86dd: ipv6;
            default: accept;
        }
    }
    state ipv6 {
        packet.extract<ipv6_t>(hdr.ipv6);
        transition select(hdr.ipv6.nextHdr) {
            8w17: parse_udp;
            8w58: icmp6;
            default: accept;
        }
    }
    state icmp6 {
        packet.extract<icmp6_t>(hdr.icmp6);
        transition accept;
    }
    state parse_udp {
        packet.extract<udp_t>(hdr.udp);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    action set_mcast_grp(bit<16> mcast_grp, bit<9> port) {
        standard_metadata.mcast_grp = mcast_grp;
        meta.egress_port = port;
    }
    table ipv6_tbl {
        key = {
            hdr.ipv6.dstAddr[127:120] == 8w0xff: exact @name("mcast_key") ;
        }
        actions = {
            set_mcast_grp();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        if (hdr.ipv6.isValid()) {
            ipv6_tbl.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    action set_out_bd(bit<24> bd) {
        meta.fwd.out_bd = bd;
    }
    table get_multicast_copy_out_bd {
        key = {
            standard_metadata.mcast_grp : exact @name("standard_metadata.mcast_grp") ;
            standard_metadata.egress_rid: exact @name("standard_metadata.egress_rid") ;
        }
        actions = {
            set_out_bd();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    action drop() {
        mark_to_drop(standard_metadata);
    }
    action rewrite_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    table send_frame {
        key = {
            meta.fwd.out_bd: exact @name("meta.fwd.out_bd") ;
        }
        actions = {
            rewrite_mac();
            drop();
        }
        default_action = drop();
    }
    apply {
        if (standard_metadata.instance_type == 32w5) {
            get_multicast_copy_out_bd.apply();
            send_frame.apply();
        }
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv6_t>(hdr.ipv6);
        packet.emit<icmp6_t>(hdr.icmp6);
        packet.emit<udp_t>(hdr.udp);
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<headers, metadata_t>(MyParser(), MyVerifyChecksum(), ingress(), egress(), MyComputeChecksum(), MyDeparser()) main;

