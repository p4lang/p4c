#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<3>   lkp_pkt_type;
    bit<48>  lkp_mac_sa;
    bit<48>  lkp_mac_da;
    bit<16>  lkp_mac_type;
    bit<2>   lkp_ip_type;
    bit<32>  lkp_ipv4_sa;
    bit<32>  lkp_ipv4_da;
    bit<128> lkp_ipv6_sa;
    bit<128> lkp_ipv6_da;
    bit<8>   lkp_ip_proto;
    bit<8>   lkp_ip_tc;
    bit<8>   lkp_ip_ttl;
    bit<16>  lkp_l4_sport;
    bit<16>  lkp_l4_dport;
    bit<16>  lkp_inner_l4_sport;
    bit<16>  lkp_inner_l4_dport;
    bit<8>   lkp_icmp_type;
    bit<8>   lkp_icmp_code;
    bit<8>   lkp_inner_icmp_type;
    bit<8>   lkp_inner_icmp_code;
    bit<16>  l3_length;
    bit<16>  ifindex;
    bit<12>  vrf;
    bit<4>   tunnel_type;
    bit<1>   tunnel_terminate;
    bit<24>  tunnel_vni;
    bit<1>   src_vtep_miss;
    bit<8>   outer_bd;
    bit<1>   outer_ipv4_mcast_key_type;
    bit<8>   outer_ipv4_mcast_key;
    bit<1>   outer_ipv6_mcast_key_type;
    bit<8>   outer_ipv6_mcast_key;
    bit<1>   outer_mcast_route_hit;
    bit<2>   outer_mcast_mode;
    bit<8>   outer_dscp;
    bit<8>   outer_ttl;
    bit<1>   outer_routed;
    bit<1>   l2_multicast;
    bit<1>   ip_multicast;
    bit<1>   src_is_link_local;
    bit<16>  bd;
    bit<1>   ipsg_enabled;
    bit<1>   ipv4_unicast_enabled;
    bit<1>   ipv6_unicast_enabled;
    bit<2>   ipv4_multicast_mode;
    bit<2>   ipv6_multicast_mode;
    bit<1>   igmp_snooping_enabled;
    bit<1>   mld_snooping_enabled;
    bit<1>   mpls_enabled;
    bit<20>  mpls_label;
    bit<3>   mpls_exp;
    bit<8>   mpls_ttl;
    bit<2>   ipv4_urpf_mode;
    bit<2>   ipv6_urpf_mode;
    bit<2>   urpf_mode;
    bit<2>   nat_mode;
    bit<10>  rmac_group;
    bit<1>   rmac_hit;
    bit<1>   mcast_route_hit;
    bit<1>   mcast_bridge_hit;
    bit<16>  bd_mrpf_group;
    bit<16>  mcast_rpf_group;
    bit<2>   mcast_mode;
    bit<16>  uuc_mc_index;
    bit<16>  umc_mc_index;
    bit<16>  bcast_mc_index;
    bit<16>  multicast_route_mc_index;
    bit<16>  multicast_bridge_mc_index;
    bit<1>   storm_control_color;
    bit<1>   urpf_hit;
    bit<1>   urpf_check_fail;
    bit<16>  urpf_bd_group;
    bit<1>   routed;
    bit<16>  if_label;
    bit<16>  bd_label;
    bit<1>   l2_src_miss;
    bit<16>  l2_src_move;
    bit<1>   ipsg_check_fail;
    bit<1>   acl_deny;
    bit<1>   racl_deny;
    bit<1>   l2_redirect;
    bit<1>   acl_redirect;
    bit<1>   racl_redirect;
    bit<1>   fib_hit;
    bit<1>   nat_hit;
    bit<10>  mirror_session_id;
    bit<16>  l2_nexthop;
    bit<16>  acl_nexthop;
    bit<16>  racl_nexthop;
    bit<16>  fib_nexthop;
    bit<16>  nat_nexthop;
    bit<1>   l2_nexthop_type;
    bit<1>   acl_nexthop_type;
    bit<1>   racl_nexthop_type;
    bit<1>   fib_nexthop_type;
    bit<16>  nexthop_index;
    bit<1>   nexthop_type;
    bit<16>  nat_rewrite_index;
    bit<3>   marked_cos;
    bit<8>   marked_dscp;
    bit<3>   marked_exp;
    bit<8>   ttl;
    bit<16>  egress_ifindex;
    bit<16>  egress_bd;
    bit<16>  same_bd_check;
    bit<1>   ingress_bypass;
    bit<24>  ipv4_dstaddr_24b;
    bit<1>   drop_0;
    bit<8>   drop_reason;
    bit<10>  stp_group;
    bit<3>   stp_state;
    bit<1>   control_frame;
    bit<4>   header_count;
}

header arp_rarp_t {
    bit<16> hwType;
    bit<16> protoType;
    bit<8>  hwAddrLen;
    bit<8>  protoAddrLen;
    bit<16> opcode;
}

header arp_rarp_ipv4_t {
    bit<48> srcHwAddr;
    bit<32> srcProtoAddr;
    bit<48> dstHwAddr;
    bit<32> dstProtoAddr;
}

header cpu_header_t {
    bit<3>  qid;
    bit<1>  pad;
    bit<12> reason_code;
    bit<16> rxhash;
    bit<16> bridge_domain;
    bit<16> ingress_lif;
    bit<16> egress_lif;
    bit<1>  lu_bypass_ingress;
    bit<1>  lu_bypass_egress;
    bit<14> pad1;
    bit<16> etherType;
}

header eompls_t {
    bit<4>  zero;
    bit<12> reserved;
    bit<16> seqNo;
}

@name("erspan_header_v1_t") header erspan_header_v1_t_0 {
    bit<4>  version;
    bit<12> vlan;
    bit<6>  priority;
    bit<10> span_id;
    bit<8>  direction;
    bit<8>  truncated;
}

@name("erspan_header_v2_t") header erspan_header_v2_t_0 {
    bit<4>  version;
    bit<12> vlan;
    bit<6>  priority;
    bit<10> span_id;
    bit<32> unknown7;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header fcoe_header_t {
    bit<4>  version;
    bit<4>  type_;
    bit<8>  sof;
    bit<32> rsvd1;
    bit<32> ts_upper;
    bit<32> ts_lower;
    bit<32> size_;
    bit<8>  eof;
    bit<24> rsvd2;
}

header genv_t {
    bit<2>  ver;
    bit<6>  optLen;
    bit<1>  oam;
    bit<1>  critical;
    bit<6>  reserved;
    bit<16> protoType;
    bit<24> vni;
    bit<8>  reserved2;
}

header genv_opt_A_t {
    bit<16> optClass;
    bit<8>  optType;
    bit<3>  reserved;
    bit<5>  optLen;
    bit<32> data;
}

header genv_opt_B_t {
    bit<16> optClass;
    bit<8>  optType;
    bit<3>  reserved;
    bit<5>  optLen;
    bit<64> data;
}

header genv_opt_C_t {
    bit<16> optClass;
    bit<8>  optType;
    bit<3>  reserved;
    bit<5>  optLen;
    bit<32> data;
}

header gre_t {
    bit<1>  C;
    bit<1>  R;
    bit<1>  K;
    bit<1>  S;
    bit<1>  s;
    bit<3>  recurse;
    bit<5>  flags;
    bit<3>  ver;
    bit<16> proto;
}

header icmp_t {
    bit<8>  type_;
    bit<8>  code;
    bit<16> hdrChecksum;
}

header icmpv6_t {
    bit<8>  type_;
    bit<8>  code;
    bit<16> hdrChecksum;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header ipv6_t {
    bit<4>   version;
    bit<8>   trafficClass;
    bit<20>  flowLabel;
    bit<16>  payloadLen;
    bit<8>   nextHdr;
    bit<8>   hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
}

header sctp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> verifTag;
    bit<32> checksum;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<8>  flags;
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

header input_port_hdr_t {
    bit<16> port;
}

header mpls_t {
    bit<20> label;
    bit<3>  tc;
    bit<1>  bos;
    bit<8>  ttl;
}

header nsh_t {
    bit<1>  oam;
    bit<1>  context;
    bit<6>  flags;
    bit<8>  reserved;
    bit<16> protoType;
    bit<24> spath;
    bit<8>  sindex;
}

header nsh_context_t {
    bit<32> network_platform;
    bit<32> network_shared;
    bit<32> service_platform;
    bit<32> service_shared;
}

header nvgre_t {
    bit<24> tni;
    bit<8>  reserved;
}

header roce_header_t {
    bit<320> ib_grh;
    bit<96>  ib_bth;
}

header roce_v2_header_t {
    bit<96> ib_bth;
}

header snap_header_t {
    bit<8>  dsap;
    bit<8>  ssap;
    bit<8>  control_;
    bit<24> oui;
    bit<16> type_;
}

header vxlan_t {
    bit<8>  flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8>  reserved2;
}

header vlan_tag_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> etherType;
}

header vlan_tag_3b_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<4>  vid;
    bit<16> etherType;
}

header vlan_tag_5b_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<20> vid;
    bit<16> etherType;
}

struct metadata {
    bit<3>   _ingress_metadata_lkp_pkt_type0;
    bit<48>  _ingress_metadata_lkp_mac_sa1;
    bit<48>  _ingress_metadata_lkp_mac_da2;
    bit<16>  _ingress_metadata_lkp_mac_type3;
    bit<2>   _ingress_metadata_lkp_ip_type4;
    bit<32>  _ingress_metadata_lkp_ipv4_sa5;
    bit<32>  _ingress_metadata_lkp_ipv4_da6;
    bit<128> _ingress_metadata_lkp_ipv6_sa7;
    bit<128> _ingress_metadata_lkp_ipv6_da8;
    bit<8>   _ingress_metadata_lkp_ip_proto9;
    bit<8>   _ingress_metadata_lkp_ip_tc10;
    bit<8>   _ingress_metadata_lkp_ip_ttl11;
    bit<16>  _ingress_metadata_lkp_l4_sport12;
    bit<16>  _ingress_metadata_lkp_l4_dport13;
    bit<16>  _ingress_metadata_lkp_inner_l4_sport14;
    bit<16>  _ingress_metadata_lkp_inner_l4_dport15;
    bit<8>   _ingress_metadata_lkp_icmp_type16;
    bit<8>   _ingress_metadata_lkp_icmp_code17;
    bit<8>   _ingress_metadata_lkp_inner_icmp_type18;
    bit<8>   _ingress_metadata_lkp_inner_icmp_code19;
    bit<16>  _ingress_metadata_l3_length20;
    bit<16>  _ingress_metadata_ifindex21;
    bit<12>  _ingress_metadata_vrf22;
    bit<4>   _ingress_metadata_tunnel_type23;
    bit<1>   _ingress_metadata_tunnel_terminate24;
    bit<24>  _ingress_metadata_tunnel_vni25;
    bit<1>   _ingress_metadata_src_vtep_miss26;
    bit<8>   _ingress_metadata_outer_bd27;
    bit<1>   _ingress_metadata_outer_ipv4_mcast_key_type28;
    bit<8>   _ingress_metadata_outer_ipv4_mcast_key29;
    bit<1>   _ingress_metadata_outer_ipv6_mcast_key_type30;
    bit<8>   _ingress_metadata_outer_ipv6_mcast_key31;
    bit<1>   _ingress_metadata_outer_mcast_route_hit32;
    bit<2>   _ingress_metadata_outer_mcast_mode33;
    bit<8>   _ingress_metadata_outer_dscp34;
    bit<8>   _ingress_metadata_outer_ttl35;
    bit<1>   _ingress_metadata_outer_routed36;
    bit<1>   _ingress_metadata_l2_multicast37;
    bit<1>   _ingress_metadata_ip_multicast38;
    bit<1>   _ingress_metadata_src_is_link_local39;
    bit<16>  _ingress_metadata_bd40;
    bit<1>   _ingress_metadata_ipsg_enabled41;
    bit<1>   _ingress_metadata_ipv4_unicast_enabled42;
    bit<1>   _ingress_metadata_ipv6_unicast_enabled43;
    bit<2>   _ingress_metadata_ipv4_multicast_mode44;
    bit<2>   _ingress_metadata_ipv6_multicast_mode45;
    bit<1>   _ingress_metadata_igmp_snooping_enabled46;
    bit<1>   _ingress_metadata_mld_snooping_enabled47;
    bit<1>   _ingress_metadata_mpls_enabled48;
    bit<20>  _ingress_metadata_mpls_label49;
    bit<3>   _ingress_metadata_mpls_exp50;
    bit<8>   _ingress_metadata_mpls_ttl51;
    bit<2>   _ingress_metadata_ipv4_urpf_mode52;
    bit<2>   _ingress_metadata_ipv6_urpf_mode53;
    bit<2>   _ingress_metadata_urpf_mode54;
    bit<2>   _ingress_metadata_nat_mode55;
    bit<10>  _ingress_metadata_rmac_group56;
    bit<1>   _ingress_metadata_rmac_hit57;
    bit<1>   _ingress_metadata_mcast_route_hit58;
    bit<1>   _ingress_metadata_mcast_bridge_hit59;
    bit<16>  _ingress_metadata_bd_mrpf_group60;
    bit<16>  _ingress_metadata_mcast_rpf_group61;
    bit<2>   _ingress_metadata_mcast_mode62;
    bit<16>  _ingress_metadata_uuc_mc_index63;
    bit<16>  _ingress_metadata_umc_mc_index64;
    bit<16>  _ingress_metadata_bcast_mc_index65;
    bit<16>  _ingress_metadata_multicast_route_mc_index66;
    bit<16>  _ingress_metadata_multicast_bridge_mc_index67;
    bit<1>   _ingress_metadata_storm_control_color68;
    bit<1>   _ingress_metadata_urpf_hit69;
    bit<1>   _ingress_metadata_urpf_check_fail70;
    bit<16>  _ingress_metadata_urpf_bd_group71;
    bit<1>   _ingress_metadata_routed72;
    bit<16>  _ingress_metadata_if_label73;
    bit<16>  _ingress_metadata_bd_label74;
    bit<1>   _ingress_metadata_l2_src_miss75;
    bit<16>  _ingress_metadata_l2_src_move76;
    bit<1>   _ingress_metadata_ipsg_check_fail77;
    bit<1>   _ingress_metadata_acl_deny78;
    bit<1>   _ingress_metadata_racl_deny79;
    bit<1>   _ingress_metadata_l2_redirect80;
    bit<1>   _ingress_metadata_acl_redirect81;
    bit<1>   _ingress_metadata_racl_redirect82;
    bit<1>   _ingress_metadata_fib_hit83;
    bit<1>   _ingress_metadata_nat_hit84;
    bit<10>  _ingress_metadata_mirror_session_id85;
    bit<16>  _ingress_metadata_l2_nexthop86;
    bit<16>  _ingress_metadata_acl_nexthop87;
    bit<16>  _ingress_metadata_racl_nexthop88;
    bit<16>  _ingress_metadata_fib_nexthop89;
    bit<16>  _ingress_metadata_nat_nexthop90;
    bit<1>   _ingress_metadata_l2_nexthop_type91;
    bit<1>   _ingress_metadata_acl_nexthop_type92;
    bit<1>   _ingress_metadata_racl_nexthop_type93;
    bit<1>   _ingress_metadata_fib_nexthop_type94;
    bit<16>  _ingress_metadata_nexthop_index95;
    bit<1>   _ingress_metadata_nexthop_type96;
    bit<16>  _ingress_metadata_nat_rewrite_index97;
    bit<3>   _ingress_metadata_marked_cos98;
    bit<8>   _ingress_metadata_marked_dscp99;
    bit<3>   _ingress_metadata_marked_exp100;
    bit<8>   _ingress_metadata_ttl101;
    bit<16>  _ingress_metadata_egress_ifindex102;
    bit<16>  _ingress_metadata_egress_bd103;
    bit<16>  _ingress_metadata_same_bd_check104;
    bit<1>   _ingress_metadata_ingress_bypass105;
    bit<24>  _ingress_metadata_ipv4_dstaddr_24b106;
    bit<1>   _ingress_metadata_drop_0107;
    bit<8>   _ingress_metadata_drop_reason108;
    bit<10>  _ingress_metadata_stp_group109;
    bit<3>   _ingress_metadata_stp_state110;
    bit<1>   _ingress_metadata_control_frame111;
    bit<4>   _ingress_metadata_header_count112;
}

struct headers {
    @name(".arp_rarp") 
    arp_rarp_t           arp_rarp;
    @name(".arp_rarp_ipv4") 
    arp_rarp_ipv4_t      arp_rarp_ipv4;
    @name(".cpu_header") 
    cpu_header_t         cpu_header;
    @name(".eompls") 
    eompls_t             eompls;
    @name(".erspan_v1_header") 
    erspan_header_v1_t_0 erspan_v1_header;
    @name(".erspan_v2_header") 
    erspan_header_v2_t_0 erspan_v2_header;
    @name(".ethernet") 
    ethernet_t           ethernet;
    @name(".fcoe") 
    fcoe_header_t        fcoe;
    @name(".genv") 
    genv_t               genv;
    @name(".genv_opt_A") 
    genv_opt_A_t         genv_opt_A;
    @name(".genv_opt_B") 
    genv_opt_B_t         genv_opt_B;
    @name(".genv_opt_C") 
    genv_opt_C_t         genv_opt_C;
    @name(".gre") 
    gre_t                gre;
    @name(".icmp") 
    icmp_t               icmp;
    @name(".icmpv6") 
    icmpv6_t             icmpv6;
    @name(".inner_ethernet") 
    ethernet_t           inner_ethernet;
    @name(".inner_icmp") 
    icmp_t               inner_icmp;
    @name(".inner_icmpv6") 
    icmpv6_t             inner_icmpv6;
    @name(".inner_ipv4") 
    ipv4_t               inner_ipv4;
    @name(".inner_ipv6") 
    ipv6_t               inner_ipv6;
    @name(".inner_sctp") 
    sctp_t               inner_sctp;
    @name(".inner_tcp") 
    tcp_t                inner_tcp;
    @name(".inner_udp") 
    udp_t                inner_udp;
    @name(".input_port_hdr") 
    input_port_hdr_t     input_port_hdr;
    @name(".ipv4") 
    ipv4_t               ipv4;
    @name(".ipv6") 
    ipv6_t               ipv6;
    @name(".mpls_bos") 
    mpls_t               mpls_bos;
    @name(".nsh") 
    nsh_t                nsh;
    @name(".nsh_context") 
    nsh_context_t        nsh_context;
    @name(".nvgre") 
    nvgre_t              nvgre;
    @name(".outer_ipv4") 
    ipv4_t               outer_ipv4;
    @name(".outer_ipv6") 
    ipv6_t               outer_ipv6;
    @name(".outer_udp") 
    udp_t                outer_udp;
    @name(".roce") 
    roce_header_t        roce;
    @name(".roce_v2") 
    roce_v2_header_t     roce_v2;
    @name(".sctp") 
    sctp_t               sctp;
    @name(".snap_header") 
    snap_header_t        snap_header;
    @name(".tcp") 
    tcp_t                tcp;
    @name(".udp") 
    udp_t                udp;
    @name(".vxlan") 
    vxlan_t              vxlan;
    @name(".mpls") 
    mpls_t[3]            mpls;
    @name(".vlan_tag_") 
    vlan_tag_t[2]        vlan_tag_;
    @name(".vlan_tag_3b") 
    vlan_tag_3b_t[2]     vlan_tag_3b;
    @name(".vlan_tag_5b") 
    vlan_tag_5b_t[2]     vlan_tag_5b;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<24> tmp;
    bit<4> tmp_0;
    @name(".parse_arp_rarp") state parse_arp_rarp {
        packet.extract<arp_rarp_t>(hdr.arp_rarp);
        transition select(hdr.arp_rarp.protoType) {
            16w0x800: parse_arp_rarp_ipv4;
            default: accept;
        }
    }
    @name(".parse_arp_rarp_ipv4") state parse_arp_rarp_ipv4 {
        packet.extract<arp_rarp_ipv4_t>(hdr.arp_rarp_ipv4);
        transition accept;
    }
    @name(".parse_cpu_header") state parse_cpu_header {
        packet.extract<cpu_header_t>(hdr.cpu_header);
        transition select(hdr.cpu_header.etherType) {
            16w0 &&& 16w0xf800: parse_snap_header;
            16w0x8100: parse_vlan;
            16w0x9100: parse_vlan;
            16w0x9200: parse_vlan;
            16w0x9300: parse_vlan;
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp_rarp;
            16w0x8035: parse_arp_rarp;
            16w0x894f: parse_nsh;
            16w0x8915: parse_roce;
            16w0x8906: parse_fcoe;
            default: accept;
        }
    }
    @name(".parse_eompls") state parse_eompls {
        packet.extract<eompls_t>(hdr.eompls);
        packet.extract<ethernet_t>(hdr.inner_ethernet);
        transition accept;
    }
    @name(".parse_erspan_v1") state parse_erspan_v1 {
        packet.extract<erspan_header_v1_t_0>(hdr.erspan_v1_header);
        transition accept;
    }
    @name(".parse_erspan_v2") state parse_erspan_v2 {
        packet.extract<erspan_header_v2_t_0>(hdr.erspan_v2_header);
        transition accept;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0 &&& 16w0xf800: parse_snap_header;
            16w0x9000: parse_cpu_header;
            16w0x10c: parse_cpu_header;
            16w0x8100: parse_vlan;
            16w0x9100: parse_vlan;
            16w0x9200: parse_vlan;
            16w0x9300: parse_vlan;
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp_rarp;
            16w0x8035: parse_arp_rarp;
            16w0x894f: parse_nsh;
            16w0x8915: parse_roce;
            16w0x8906: parse_fcoe;
            default: accept;
        }
    }
    @name(".parse_fcoe") state parse_fcoe {
        packet.extract<fcoe_header_t>(hdr.fcoe);
        transition accept;
    }
    @name(".parse_geneve") state parse_geneve {
        packet.extract<genv_t>(hdr.genv);
        transition parse_genv_inner;
    }
    @name(".parse_genv_inner") state parse_genv_inner {
        transition select(hdr.genv.protoType) {
            16w0x6558: parse_inner_ethernet;
            16w0x800: parse_inner_ipv4;
            16w0x86dd: parse_inner_ipv6;
            default: accept;
        }
    }
    @name(".parse_gre") state parse_gre {
        packet.extract<gre_t>(hdr.gre);
        transition select(hdr.gre.K, hdr.gre.proto) {
            (1w0x0, 16w0x6558): parse_nvgre;
            (1w0x0, 16w0x88be): parse_erspan_v1;
            (1w0x0, 16w0x22eb): parse_erspan_v2;
            (1w0x0, 16w0x894f): parse_nsh;
            default: accept;
        }
    }
    @name(".parse_icmp") state parse_icmp {
        packet.extract<icmp_t>(hdr.icmp);
        transition accept;
    }
    @name(".parse_icmpv6") state parse_icmpv6 {
        packet.extract<icmpv6_t>(hdr.icmpv6);
        transition accept;
    }
    @name(".parse_inner_ethernet") state parse_inner_ethernet {
        packet.extract<ethernet_t>(hdr.inner_ethernet);
        transition select(hdr.inner_ethernet.etherType) {
            16w0x800: parse_inner_ipv4;
            16w0x86dd: parse_inner_ipv6;
            default: accept;
        }
    }
    @name(".parse_inner_icmp") state parse_inner_icmp {
        packet.extract<icmp_t>(hdr.inner_icmp);
        transition accept;
    }
    @name(".parse_inner_icmpv6") state parse_inner_icmpv6 {
        packet.extract<icmpv6_t>(hdr.inner_icmpv6);
        transition accept;
    }
    @name(".parse_inner_ipv4") state parse_inner_ipv4 {
        packet.extract<ipv4_t>(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.protocol) {
            (13w0, 8w1): parse_inner_icmp;
            (13w0, 8w6): parse_inner_tcp;
            (13w0, 8w17): parse_inner_udp;
            default: accept;
        }
    }
    @name(".parse_inner_ipv6") state parse_inner_ipv6 {
        packet.extract<ipv6_t>(hdr.inner_ipv6);
        transition select(hdr.inner_ipv6.nextHdr) {
            8w58: parse_inner_icmpv6;
            8w6: parse_inner_tcp;
            8w17: parse_inner_udp;
            default: accept;
        }
    }
    @name(".parse_inner_tcp") state parse_inner_tcp {
        packet.extract<tcp_t>(hdr.inner_tcp);
        transition accept;
    }
    @name(".parse_inner_udp") state parse_inner_udp {
        packet.extract<udp_t>(hdr.inner_udp);
        transition accept;
    }
    @name(".parse_input_port") state parse_input_port {
        packet.extract<input_port_hdr_t>(hdr.input_port_hdr);
        transition parse_ethernet;
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.fragOffset, hdr.ipv4.protocol) {
            (13w0, 8w1): parse_icmp;
            (13w0, 8w6): parse_tcp;
            (13w0, 8w17): parse_udp;
            (13w0, 8w47): parse_gre;
            default: accept;
        }
    }
    @name(".parse_ipv6") state parse_ipv6 {
        packet.extract<ipv6_t>(hdr.ipv6);
        transition select(hdr.ipv6.nextHdr) {
            8w58: parse_icmpv6;
            8w6: parse_tcp;
            8w17: parse_udp;
            8w47: parse_gre;
            default: accept;
        }
    }
    @name(".parse_mpls") state parse_mpls {
        tmp = packet.lookahead<bit<24>>();
        transition select(tmp[0:0]) {
            1w0: parse_mpls_not_bos;
            1w1: parse_mpls_bos;
            default: accept;
        }
    }
    @name(".parse_mpls_bos") state parse_mpls_bos {
        packet.extract<mpls_t>(hdr.mpls_bos);
        tmp_0 = packet.lookahead<bit<4>>();
        transition select(tmp_0[3:0]) {
            4w0x4: parse_inner_ipv4;
            4w0x6: parse_inner_ipv6;
            default: parse_eompls;
        }
    }
    @name(".parse_mpls_not_bos") state parse_mpls_not_bos {
        packet.extract<mpls_t>(hdr.mpls.next);
        transition parse_mpls;
    }
    @name(".parse_nsh") state parse_nsh {
        packet.extract<nsh_t>(hdr.nsh);
        packet.extract<nsh_context_t>(hdr.nsh_context);
        transition select(hdr.nsh.protoType) {
            16w0x800: parse_inner_ipv4;
            16w0x86dd: parse_inner_ipv6;
            16w0x6558: parse_inner_ethernet;
            default: accept;
        }
    }
    @name(".parse_nvgre") state parse_nvgre {
        packet.extract<nvgre_t>(hdr.nvgre);
        transition parse_inner_ethernet;
    }
    @name(".parse_roce") state parse_roce {
        packet.extract<roce_header_t>(hdr.roce);
        transition accept;
    }
    @name(".parse_roce_v2") state parse_roce_v2 {
        packet.extract<roce_v2_header_t>(hdr.roce_v2);
        transition accept;
    }
    @name(".parse_snap_header") state parse_snap_header {
        packet.extract<snap_header_t>(hdr.snap_header);
        transition accept;
    }
    @name(".parse_tcp") state parse_tcp {
        packet.extract<tcp_t>(hdr.tcp);
        transition accept;
    }
    @name(".parse_udp") state parse_udp {
        packet.extract<udp_t>(hdr.udp);
        transition select(hdr.udp.dstPort) {
            16w4789: parse_vxlan;
            16w6081: parse_geneve;
            16w1021: parse_roce_v2;
            default: accept;
        }
    }
    @name(".parse_vlan") state parse_vlan {
        packet.extract<vlan_tag_t>(hdr.vlan_tag_.next);
        transition select(hdr.vlan_tag_.last.etherType) {
            16w0x8100: parse_vlan;
            16w0x9100: parse_vlan;
            16w0x9200: parse_vlan;
            16w0x9300: parse_vlan;
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp_rarp;
            16w0x8035: parse_arp_rarp;
            default: accept;
        }
    }
    @name(".parse_vxlan") state parse_vxlan {
        packet.extract<vxlan_t>(hdr.vxlan);
        transition parse_inner_ethernet;
    }
    @name(".start") state start {
        transition parse_input_port;
    }
}

@name(".outer_bd_action_profile") action_profile(32w256) outer_bd_action_profile;

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".set_bd") action set_bd(bit<16> outer_vlan_bd, bit<12> vrf, bit<10> rmac_group, bit<16> bd_label, bit<16> uuc_mc_index, bit<16> bcast_mc_index, bit<16> umc_mc_index, bit<1> ipv4_unicast_enabled, bit<1> igmp_snooping_enabled, bit<10> stp_group) {
        meta._ingress_metadata_vrf22 = vrf;
        meta._ingress_metadata_ipv4_unicast_enabled42 = ipv4_unicast_enabled;
        meta._ingress_metadata_igmp_snooping_enabled46 = igmp_snooping_enabled;
        meta._ingress_metadata_rmac_group56 = rmac_group;
        meta._ingress_metadata_uuc_mc_index63 = uuc_mc_index;
        meta._ingress_metadata_umc_mc_index64 = umc_mc_index;
        meta._ingress_metadata_bcast_mc_index65 = bcast_mc_index;
        meta._ingress_metadata_bd_label74 = bd_label;
        meta._ingress_metadata_bd40 = outer_vlan_bd;
        meta._ingress_metadata_stp_group109 = stp_group;
    }
    @name(".set_outer_bd_ipv4_mcast_switch_ipv6_mcast_switch_flags") action set_outer_bd_ipv4_mcast_switch_ipv6_mcast_switch_flags(bit<16> bd, bit<12> vrf, bit<10> rmac_group, bit<16> mrpf_group, bit<16> bd_label, bit<16> uuc_mc_index, bit<16> bcast_mc_index, bit<16> umc_mc_index, bit<1> ipv4_unicast_enabled, bit<1> ipv6_unicast_enabled, bit<2> ipv4_multicast_mode, bit<2> ipv6_multicast_mode, bit<1> igmp_snooping_enabled, bit<1> mld_snooping_enabled, bit<2> ipv4_urpf_mode, bit<2> ipv6_urpf_mode, bit<10> stp_group) {
        meta._ingress_metadata_vrf22 = vrf;
        meta._ingress_metadata_bd40 = bd;
        meta._ingress_metadata_outer_bd27 = (bit<8>)bd;
        meta._ingress_metadata_outer_ipv4_mcast_key_type28 = 1w0;
        meta._ingress_metadata_outer_ipv4_mcast_key29 = (bit<8>)bd;
        meta._ingress_metadata_outer_ipv6_mcast_key_type30 = 1w0;
        meta._ingress_metadata_outer_ipv6_mcast_key31 = (bit<8>)bd;
        meta._ingress_metadata_ipv4_unicast_enabled42 = ipv4_unicast_enabled;
        meta._ingress_metadata_ipv6_unicast_enabled43 = ipv6_unicast_enabled;
        meta._ingress_metadata_ipv4_multicast_mode44 = ipv4_multicast_mode;
        meta._ingress_metadata_ipv6_multicast_mode45 = ipv6_multicast_mode;
        meta._ingress_metadata_igmp_snooping_enabled46 = igmp_snooping_enabled;
        meta._ingress_metadata_mld_snooping_enabled47 = mld_snooping_enabled;
        meta._ingress_metadata_ipv4_urpf_mode52 = ipv4_urpf_mode;
        meta._ingress_metadata_ipv6_urpf_mode53 = ipv6_urpf_mode;
        meta._ingress_metadata_rmac_group56 = rmac_group;
        meta._ingress_metadata_bd_mrpf_group60 = mrpf_group;
        meta._ingress_metadata_uuc_mc_index63 = uuc_mc_index;
        meta._ingress_metadata_umc_mc_index64 = umc_mc_index;
        meta._ingress_metadata_bcast_mc_index65 = bcast_mc_index;
        meta._ingress_metadata_bd_label74 = bd_label;
        meta._ingress_metadata_stp_group109 = stp_group;
    }
    @name(".set_outer_bd_ipv4_mcast_switch_ipv6_mcast_route_flags") action set_outer_bd_ipv4_mcast_switch_ipv6_mcast_route_flags(bit<16> bd, bit<12> vrf, bit<10> rmac_group, bit<16> mrpf_group, bit<16> bd_label, bit<16> uuc_mc_index, bit<16> bcast_mc_index, bit<16> umc_mc_index, bit<1> ipv4_unicast_enabled, bit<1> ipv6_unicast_enabled, bit<2> ipv4_multicast_mode, bit<2> ipv6_multicast_mode, bit<1> igmp_snooping_enabled, bit<1> mld_snooping_enabled, bit<2> ipv4_urpf_mode, bit<2> ipv6_urpf_mode, bit<10> stp_group) {
        meta._ingress_metadata_vrf22 = vrf;
        meta._ingress_metadata_bd40 = bd;
        meta._ingress_metadata_outer_bd27 = (bit<8>)bd;
        meta._ingress_metadata_outer_ipv4_mcast_key_type28 = 1w0;
        meta._ingress_metadata_outer_ipv4_mcast_key29 = (bit<8>)bd;
        meta._ingress_metadata_outer_ipv6_mcast_key_type30 = 1w1;
        meta._ingress_metadata_outer_ipv6_mcast_key31 = (bit<8>)vrf;
        meta._ingress_metadata_ipv4_unicast_enabled42 = ipv4_unicast_enabled;
        meta._ingress_metadata_ipv6_unicast_enabled43 = ipv6_unicast_enabled;
        meta._ingress_metadata_ipv4_multicast_mode44 = ipv4_multicast_mode;
        meta._ingress_metadata_ipv6_multicast_mode45 = ipv6_multicast_mode;
        meta._ingress_metadata_igmp_snooping_enabled46 = igmp_snooping_enabled;
        meta._ingress_metadata_mld_snooping_enabled47 = mld_snooping_enabled;
        meta._ingress_metadata_ipv4_urpf_mode52 = ipv4_urpf_mode;
        meta._ingress_metadata_ipv6_urpf_mode53 = ipv6_urpf_mode;
        meta._ingress_metadata_rmac_group56 = rmac_group;
        meta._ingress_metadata_bd_mrpf_group60 = mrpf_group;
        meta._ingress_metadata_uuc_mc_index63 = uuc_mc_index;
        meta._ingress_metadata_umc_mc_index64 = umc_mc_index;
        meta._ingress_metadata_bcast_mc_index65 = bcast_mc_index;
        meta._ingress_metadata_bd_label74 = bd_label;
        meta._ingress_metadata_stp_group109 = stp_group;
    }
    @name(".set_outer_bd_ipv4_mcast_route_ipv6_mcast_switch_flags") action set_outer_bd_ipv4_mcast_route_ipv6_mcast_switch_flags(bit<16> bd, bit<12> vrf, bit<10> rmac_group, bit<16> mrpf_group, bit<16> bd_label, bit<16> uuc_mc_index, bit<16> bcast_mc_index, bit<16> umc_mc_index, bit<1> ipv4_unicast_enabled, bit<1> ipv6_unicast_enabled, bit<2> ipv4_multicast_mode, bit<2> ipv6_multicast_mode, bit<1> igmp_snooping_enabled, bit<1> mld_snooping_enabled, bit<2> ipv4_urpf_mode, bit<2> ipv6_urpf_mode, bit<10> stp_group) {
        meta._ingress_metadata_vrf22 = vrf;
        meta._ingress_metadata_bd40 = bd;
        meta._ingress_metadata_outer_bd27 = (bit<8>)bd;
        meta._ingress_metadata_outer_ipv4_mcast_key_type28 = 1w1;
        meta._ingress_metadata_outer_ipv4_mcast_key29 = (bit<8>)vrf;
        meta._ingress_metadata_outer_ipv6_mcast_key_type30 = 1w0;
        meta._ingress_metadata_outer_ipv6_mcast_key31 = (bit<8>)bd;
        meta._ingress_metadata_ipv4_unicast_enabled42 = ipv4_unicast_enabled;
        meta._ingress_metadata_ipv6_unicast_enabled43 = ipv6_unicast_enabled;
        meta._ingress_metadata_ipv4_multicast_mode44 = ipv4_multicast_mode;
        meta._ingress_metadata_ipv6_multicast_mode45 = ipv6_multicast_mode;
        meta._ingress_metadata_igmp_snooping_enabled46 = igmp_snooping_enabled;
        meta._ingress_metadata_mld_snooping_enabled47 = mld_snooping_enabled;
        meta._ingress_metadata_ipv4_urpf_mode52 = ipv4_urpf_mode;
        meta._ingress_metadata_ipv6_urpf_mode53 = ipv6_urpf_mode;
        meta._ingress_metadata_rmac_group56 = rmac_group;
        meta._ingress_metadata_bd_mrpf_group60 = mrpf_group;
        meta._ingress_metadata_uuc_mc_index63 = uuc_mc_index;
        meta._ingress_metadata_umc_mc_index64 = umc_mc_index;
        meta._ingress_metadata_bcast_mc_index65 = bcast_mc_index;
        meta._ingress_metadata_bd_label74 = bd_label;
        meta._ingress_metadata_stp_group109 = stp_group;
    }
    @name(".set_outer_bd_ipv4_mcast_route_ipv6_mcast_route_flags") action set_outer_bd_ipv4_mcast_route_ipv6_mcast_route_flags(bit<16> bd, bit<12> vrf, bit<10> rmac_group, bit<16> mrpf_group, bit<16> bd_label, bit<16> uuc_mc_index, bit<16> bcast_mc_index, bit<16> umc_mc_index, bit<1> ipv4_unicast_enabled, bit<1> ipv6_unicast_enabled, bit<2> ipv4_multicast_mode, bit<2> ipv6_multicast_mode, bit<1> igmp_snooping_enabled, bit<1> mld_snooping_enabled, bit<2> ipv4_urpf_mode, bit<2> ipv6_urpf_mode, bit<10> stp_group) {
        meta._ingress_metadata_vrf22 = vrf;
        meta._ingress_metadata_bd40 = bd;
        meta._ingress_metadata_outer_bd27 = (bit<8>)bd;
        meta._ingress_metadata_outer_ipv4_mcast_key_type28 = 1w1;
        meta._ingress_metadata_outer_ipv4_mcast_key29 = (bit<8>)vrf;
        meta._ingress_metadata_outer_ipv6_mcast_key_type30 = 1w1;
        meta._ingress_metadata_outer_ipv6_mcast_key31 = (bit<8>)vrf;
        meta._ingress_metadata_ipv4_unicast_enabled42 = ipv4_unicast_enabled;
        meta._ingress_metadata_ipv6_unicast_enabled43 = ipv6_unicast_enabled;
        meta._ingress_metadata_ipv4_multicast_mode44 = ipv4_multicast_mode;
        meta._ingress_metadata_ipv6_multicast_mode45 = ipv6_multicast_mode;
        meta._ingress_metadata_igmp_snooping_enabled46 = igmp_snooping_enabled;
        meta._ingress_metadata_mld_snooping_enabled47 = mld_snooping_enabled;
        meta._ingress_metadata_ipv4_urpf_mode52 = ipv4_urpf_mode;
        meta._ingress_metadata_ipv6_urpf_mode53 = ipv6_urpf_mode;
        meta._ingress_metadata_rmac_group56 = rmac_group;
        meta._ingress_metadata_bd_mrpf_group60 = mrpf_group;
        meta._ingress_metadata_uuc_mc_index63 = uuc_mc_index;
        meta._ingress_metadata_umc_mc_index64 = umc_mc_index;
        meta._ingress_metadata_bcast_mc_index65 = bcast_mc_index;
        meta._ingress_metadata_bd_label74 = bd_label;
        meta._ingress_metadata_stp_group109 = stp_group;
    }
    @name(".port_vlan_mapping") table port_vlan_mapping_0 {
        actions = {
            set_bd();
            set_outer_bd_ipv4_mcast_switch_ipv6_mcast_switch_flags();
            set_outer_bd_ipv4_mcast_switch_ipv6_mcast_route_flags();
            set_outer_bd_ipv4_mcast_route_ipv6_mcast_switch_flags();
            set_outer_bd_ipv4_mcast_route_ipv6_mcast_route_flags();
            @defaultonly NoAction_0();
        }
        key = {
            meta._ingress_metadata_ifindex21: exact @name("ingress_metadata.ifindex") ;
            hdr.vlan_tag_[0].isValid()      : exact @name("vlan_tag_[0].$valid$") ;
            hdr.vlan_tag_[0].vid            : exact @name("vlan_tag_[0].vid") ;
            hdr.vlan_tag_[1].isValid()      : exact @name("vlan_tag_[1].$valid$") ;
            hdr.vlan_tag_[1].vid            : exact @name("vlan_tag_[1].vid") ;
        }
        size = 32768;
        implementation = outer_bd_action_profile;
        default_action = NoAction_0();
    }
    apply {
        port_vlan_mapping_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<input_port_hdr_t>(hdr.input_port_hdr);
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<cpu_header_t>(hdr.cpu_header);
        packet.emit<fcoe_header_t>(hdr.fcoe);
        packet.emit<roce_header_t>(hdr.roce);
        packet.emit<vlan_tag_t>(hdr.vlan_tag_[0]);
        packet.emit<vlan_tag_t>(hdr.vlan_tag_[1]);
        packet.emit<arp_rarp_t>(hdr.arp_rarp);
        packet.emit<arp_rarp_ipv4_t>(hdr.arp_rarp_ipv4);
        packet.emit<ipv6_t>(hdr.ipv6);
        packet.emit<icmpv6_t>(hdr.icmpv6);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<gre_t>(hdr.gre);
        packet.emit<nsh_t>(hdr.nsh);
        packet.emit<nsh_context_t>(hdr.nsh_context);
        packet.emit<erspan_header_v2_t_0>(hdr.erspan_v2_header);
        packet.emit<erspan_header_v1_t_0>(hdr.erspan_v1_header);
        packet.emit<nvgre_t>(hdr.nvgre);
        packet.emit<udp_t>(hdr.udp);
        packet.emit<roce_v2_header_t>(hdr.roce_v2);
        packet.emit<genv_t>(hdr.genv);
        packet.emit<vxlan_t>(hdr.vxlan);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<icmp_t>(hdr.icmp);
        packet.emit<mpls_t>(hdr.mpls[0]);
        packet.emit<mpls_t>(hdr.mpls[1]);
        packet.emit<mpls_t>(hdr.mpls[2]);
        packet.emit<mpls_t>(hdr.mpls_bos);
        packet.emit<eompls_t>(hdr.eompls);
        packet.emit<ethernet_t>(hdr.inner_ethernet);
        packet.emit<ipv6_t>(hdr.inner_ipv6);
        packet.emit<icmpv6_t>(hdr.inner_icmpv6);
        packet.emit<ipv4_t>(hdr.inner_ipv4);
        packet.emit<udp_t>(hdr.inner_udp);
        packet.emit<tcp_t>(hdr.inner_tcp);
        packet.emit<icmp_t>(hdr.inner_icmp);
        packet.emit<snap_header_t>(hdr.snap_header);
    }
}

struct tuple_0 {
    bit<4>  field;
    bit<4>  field_0;
    bit<8>  field_1;
    bit<16> field_2;
    bit<16> field_3;
    bit<3>  field_4;
    bit<13> field_5;
    bit<8>  field_6;
    bit<8>  field_7;
    bit<32> field_8;
    bit<32> field_9;
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr }, hdr.inner_ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum<tuple_0, bit<16>>(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr }, hdr.inner_ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

