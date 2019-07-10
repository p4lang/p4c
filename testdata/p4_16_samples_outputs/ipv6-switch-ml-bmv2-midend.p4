#include <core.p4>
#include <v1model.p4>

typedef bit<48> mac_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<9> port_t;
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
    bit<9>   _ingress_port0;
    bit<9>   _egress_port1;
    bit<16>  _tcp_length2;
    bit<32>  _cast_length3;
    bit<1>   _do_cksum4;
    bit<48>  _l2_metadata_lkp_mac_sa5;
    bit<48>  _l2_metadata_lkp_mac_da6;
    bit<3>   _l2_metadata_lkp_pkt_type7;
    bit<16>  _l2_metadata_lkp_mac_type8;
    bit<3>   _l2_metadata_lkp_pcp9;
    bit<16>  _l2_metadata_l2_nexthop10;
    bit<2>   _l2_metadata_l2_nexthop_type11;
    bit<1>   _l2_metadata_l2_redirect12;
    bit<1>   _l2_metadata_l2_src_miss13;
    bit<16>  _l2_metadata_l2_src_move14;
    bit<10>  _l2_metadata_stp_group15;
    bit<3>   _l2_metadata_stp_state16;
    bit<16>  _l2_metadata_bd_stats_idx17;
    bit<1>   _l2_metadata_learning_enabled18;
    bit<1>   _l2_metadata_port_vlan_mapping_miss19;
    bit<16>  _l2_metadata_same_if_check20;
    bit<2>   _l3_metadata_lkp_ip_type21;
    bit<4>   _l3_metadata_lkp_ip_version22;
    bit<8>   _l3_metadata_lkp_ip_proto23;
    bit<8>   _l3_metadata_lkp_dscp24;
    bit<8>   _l3_metadata_lkp_ip_ttl25;
    bit<16>  _l3_metadata_lkp_l4_sport26;
    bit<16>  _l3_metadata_lkp_l4_dport27;
    bit<16>  _l3_metadata_lkp_outer_l4_sport28;
    bit<16>  _l3_metadata_lkp_outer_l4_dport29;
    bit<16>  _l3_metadata_vrf30;
    bit<10>  _l3_metadata_rmac_group31;
    bit<1>   _l3_metadata_rmac_hit32;
    bit<2>   _l3_metadata_urpf_mode33;
    bit<1>   _l3_metadata_urpf_hit34;
    bit<1>   _l3_metadata_urpf_check_fail35;
    bit<16>  _l3_metadata_urpf_bd_group36;
    bit<1>   _l3_metadata_fib_hit37;
    bit<16>  _l3_metadata_fib_nexthop38;
    bit<2>   _l3_metadata_fib_nexthop_type39;
    bit<16>  _l3_metadata_same_bd_check40;
    bit<16>  _l3_metadata_nexthop_index41;
    bit<1>   _l3_metadata_routed42;
    bit<1>   _l3_metadata_outer_routed43;
    bit<8>   _l3_metadata_mtu_index44;
    bit<1>   _l3_metadata_l3_copy45;
    bit<16>  _l3_metadata_l3_mtu_check46;
    bit<16>  _l3_metadata_egress_l4_sport47;
    bit<16>  _l3_metadata_egress_l4_dport48;
    bit<128> _ipv6_metadata_lkp_ipv6_sa49;
    bit<128> _ipv6_metadata_lkp_ipv6_da50;
    bit<1>   _ipv6_metadata_ipv6_unicast_enabled51;
    bit<1>   _ipv6_metadata_ipv6_src_is_link_local52;
    bit<2>   _ipv6_metadata_ipv6_urpf_mode53;
    bit<5>   _tunnel_metadata_ingress_tunnel_type54;
    bit<24>  _tunnel_metadata_tunnel_vni55;
    bit<5>   _tunnel_metadata_egress_tunnel_type56;
    bit<14>  _tunnel_metadata_tunnel_index57;
    bit<9>   _tunnel_metadata_tunnel_src_index58;
    bit<9>   _tunnel_metadata_tunnel_smac_index59;
    bit<14>  _tunnel_metadata_tunnel_dst_index60;
    bit<14>  _tunnel_metadata_tunnel_dmac_index61;
    bit<24>  _tunnel_metadata_vnid62;
    bit<1>   _tunnel_metadata_tunnel_terminate63;
    bit<1>   _tunnel_metadata_tunnel_if_check64;
    bit<4>   _tunnel_metadata_egress_header_count65;
    bit<8>   _tunnel_metadata_inner_ip_proto66;
    bit<1>   _tunnel_metadata_skip_encap_inner67;
    bit<32>  _fwd_l2ptr68;
    bit<24>  _fwd_out_bd69;
}

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
    bool key_0;
    @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.set_mcast_grp") action set_mcast_grp(bit<16> mcast_grp, bit<9> port) {
        standard_metadata.mcast_grp = mcast_grp;
        meta._egress_port1 = port;
    }
    @name("ingress.ipv6_tbl") table ipv6_tbl_0 {
        key = {
            key_0: exact @name("mcast_key") ;
        }
        actions = {
            set_mcast_grp();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action act() {
        key_0 = hdr.ipv6.dstAddr[127:120] == 8w0xff;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        if (hdr.ipv6.isValid()) {
            tbl_act.apply();
            ipv6_tbl_0.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_1() {
    }
    @name("egress.set_out_bd") action set_out_bd(bit<24> bd) {
        meta._fwd_out_bd69 = bd;
    }
    @name("egress.get_multicast_copy_out_bd") table get_multicast_copy_out_bd_0 {
        key = {
            standard_metadata.mcast_grp : exact @name("standard_metadata.mcast_grp") ;
            standard_metadata.egress_rid: exact @name("standard_metadata.egress_rid") ;
        }
        actions = {
            set_out_bd();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("egress.drop") action drop() {
        mark_to_drop(standard_metadata);
    }
    @name("egress.rewrite_mac") action rewrite_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("egress.send_frame") table send_frame_0 {
        key = {
            meta._fwd_out_bd69: exact @name("meta.fwd.out_bd") ;
        }
        actions = {
            rewrite_mac();
            drop();
        }
        default_action = drop();
    }
    apply {
        if (standard_metadata.instance_type == 32w5) {
            get_multicast_copy_out_bd_0.apply();
            send_frame_0.apply();
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

