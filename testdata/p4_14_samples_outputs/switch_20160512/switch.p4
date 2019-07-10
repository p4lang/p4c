#include <core.p4>
#include <v1model.p4>

struct acl_metadata_t {
    bit<1>  acl_deny;
    bit<1>  acl_copy;
    bit<1>  racl_deny;
    bit<16> acl_nexthop;
    bit<16> racl_nexthop;
    bit<1>  acl_nexthop_type;
    bit<1>  racl_nexthop_type;
    bit<1>  acl_redirect;
    bit<1>  racl_redirect;
    bit<16> if_label;
    bit<16> bd_label;
    bit<14> acl_stats_index;
}

struct egress_filter_metadata_t {
    bit<16> ifindex_check;
    bit<16> bd;
    bit<16> inner_bd;
}

struct egress_metadata_t {
    bit<1>  bypass;
    bit<2>  port_type;
    bit<16> payload_length;
    bit<9>  smac_idx;
    bit<16> bd;
    bit<16> outer_bd;
    bit<48> mac_da;
    bit<1>  routed;
    bit<16> same_bd_check;
    bit<8>  drop_reason;
    bit<16> ifindex;
}

struct fabric_metadata_t {
    bit<3>  packetType;
    bit<1>  fabric_header_present;
    bit<16> reason_code;
    bit<8>  dst_device;
    bit<16> dst_port;
}

struct global_config_metadata_t {
    bit<1> enable_dod;
}

struct hash_metadata_t {
    bit<16> hash1;
    bit<16> hash2;
    bit<16> entropy_hash;
}

struct i2e_metadata_t {
    bit<32> ingress_tstamp;
    bit<16> mirror_session_id;
}

struct ingress_metadata_t {
    bit<9>  ingress_port;
    bit<16> ifindex;
    bit<16> egress_ifindex;
    bit<2>  port_type;
    bit<16> outer_bd;
    bit<16> bd;
    bit<1>  drop_flag;
    bit<8>  drop_reason;
    bit<1>  control_frame;
    bit<16> bypass_lookups;
    @saturating 
    bit<32> sflow_take_sample;
}

struct int_metadata_t {
    bit<32> switch_id;
    bit<8>  insert_cnt;
    bit<16> insert_byte_cnt;
    bit<16> gpe_int_hdr_len;
    bit<8>  gpe_int_hdr_len8;
    bit<16> instruction_cnt;
}

struct int_metadata_i2e_t {
    bit<1> sink;
    bit<1> source;
}

struct ingress_intrinsic_metadata_t {
    bit<48> ingress_global_timestamp;
    bit<16> mcast_grp;
    bit<16> egress_rid;
    bit<3>  priority;
}

struct ipv4_metadata_t {
    bit<32> lkp_ipv4_sa;
    bit<32> lkp_ipv4_da;
    bit<1>  ipv4_unicast_enabled;
    bit<2>  ipv4_urpf_mode;
}

struct ipv6_metadata_t {
    bit<128> lkp_ipv6_sa;
    bit<128> lkp_ipv6_da;
    bit<1>   ipv6_unicast_enabled;
    bit<1>   ipv6_src_is_link_local;
    bit<2>   ipv6_urpf_mode;
}

struct l2_metadata_t {
    bit<48> lkp_mac_sa;
    bit<48> lkp_mac_da;
    bit<3>  lkp_pkt_type;
    bit<16> lkp_mac_type;
    bit<16> l2_nexthop;
    bit<1>  l2_nexthop_type;
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

struct l3_metadata_t {
    bit<2>  lkp_ip_type;
    bit<4>  lkp_ip_version;
    bit<8>  lkp_ip_proto;
    bit<8>  lkp_ip_tc;
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
    bit<1>  fib_nexthop_type;
    bit<16> same_bd_check;
    bit<16> nexthop_index;
    bit<1>  routed;
    bit<1>  outer_routed;
    bit<8>  mtu_index;
    bit<1>  l3_copy;
    @saturating 
    bit<16> l3_mtu_check;
}

struct meter_metadata_t {
    bit<2>  meter_color;
    bit<16> meter_index;
}

struct multicast_metadata_t {
    bit<1>  ipv4_mcast_key_type;
    bit<16> ipv4_mcast_key;
    bit<1>  ipv6_mcast_key_type;
    bit<16> ipv6_mcast_key;
    bit<1>  outer_mcast_route_hit;
    bit<2>  outer_mcast_mode;
    bit<1>  mcast_route_hit;
    bit<1>  mcast_bridge_hit;
    bit<1>  ipv4_multicast_enabled;
    bit<1>  ipv6_multicast_enabled;
    bit<1>  igmp_snooping_enabled;
    bit<1>  mld_snooping_enabled;
    bit<16> bd_mrpf_group;
    bit<16> mcast_rpf_group;
    bit<2>  mcast_mode;
    bit<16> multicast_route_mc_index;
    bit<16> multicast_bridge_mc_index;
    bit<1>  inner_replica;
    bit<1>  replica;
    bit<16> mcast_grp;
}

struct nexthop_metadata_t {
    bit<1> nexthop_type;
}

struct qos_metadata_t {
    bit<8> outer_dscp;
    bit<3> marked_cos;
    bit<8> marked_dscp;
    bit<3> marked_exp;
}

struct queueing_metadata_t {
    bit<48> enq_timestamp;
    bit<16> enq_qdepth;
    bit<32> deq_timedelta;
    bit<16> deq_qdepth;
}

struct security_metadata_t {
    bit<1> storm_control_color;
    bit<1> ipsg_enabled;
    bit<1> ipsg_check_fail;
}

struct sflow_meta_t {
    bit<16> sflow_session_id;
}

struct tunnel_metadata_t {
    bit<5>  ingress_tunnel_type;
    bit<24> tunnel_vni;
    bit<1>  mpls_enabled;
    bit<20> mpls_label;
    bit<3>  mpls_exp;
    bit<8>  mpls_ttl;
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

header bfd_t {
    bit<3>  version;
    bit<5>  diag;
    bit<2>  state;
    bit<1>  p;
    bit<1>  f;
    bit<1>  c;
    bit<1>  a;
    bit<1>  d;
    bit<1>  m;
    bit<8>  detectMult;
    bit<8>  len;
    bit<32> myDiscriminator;
    bit<32> yourDiscriminator;
    bit<32> desiredMinTxInterval;
    bit<32> requiredMinRxInterval;
    bit<32> requiredMinEchoRxInterval;
}

header eompls_t {
    bit<4>  zero;
    bit<12> reserved;
    bit<16> seqNo;
}

@name("erspan_header_t3_t") header erspan_header_t3_t_0 {
    bit<4>  version;
    bit<12> vlan;
    bit<6>  priority;
    bit<10> span_id;
    bit<32> timestamp;
    bit<32> sgt_other;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header fabric_header_t {
    bit<3>  packetType;
    bit<2>  headerVersion;
    bit<2>  packetVersion;
    bit<1>  pad1;
    bit<3>  fabricColor;
    bit<5>  fabricQos;
    bit<8>  dstDevice;
    bit<16> dstPortOrGroup;
}

header fabric_header_cpu_t {
    bit<5>  egressQueue;
    bit<1>  txBypass;
    bit<2>  reserved;
    bit<16> ingressPort;
    bit<16> ingressIfindex;
    bit<16> ingressBd;
    bit<16> reasonCode;
}

header fabric_header_mirror_t {
    bit<16> rewriteIndex;
    bit<10> egressPort;
    bit<5>  egressQueue;
    bit<1>  pad;
}

header fabric_header_multicast_t {
    bit<1>  routed;
    bit<1>  outerRouted;
    bit<1>  tunnelTerminate;
    bit<5>  ingressTunnelType;
    bit<16> ingressIfindex;
    bit<16> ingressBd;
    bit<16> mcastGrp;
}

header fabric_header_sflow_t {
    bit<16> sflow_session_id;
}

header fabric_header_unicast_t {
    bit<1>  routed;
    bit<1>  outerRouted;
    bit<1>  tunnelTerminate;
    bit<5>  ingressTunnelType;
    bit<16> nexthopIndex;
}

header fabric_payload_header_t {
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
    bit<16> typeCode;
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

header int_egress_port_id_header_t {
    bit<1>  bos;
    bit<31> egress_port_id;
}

header int_egress_port_tx_utilization_header_t {
    bit<1>  bos;
    bit<31> egress_port_tx_utilization;
}

header int_header_t {
    bit<2>  ver;
    bit<2>  rep;
    bit<1>  c;
    bit<1>  e;
    bit<5>  rsvd1;
    bit<5>  ins_cnt;
    bit<8>  max_hop_cnt;
    bit<8>  total_hop_cnt;
    bit<4>  instruction_mask_0003;
    bit<4>  instruction_mask_0407;
    bit<4>  instruction_mask_0811;
    bit<4>  instruction_mask_1215;
    bit<16> rsvd2;
}

header int_hop_latency_header_t {
    bit<1>  bos;
    bit<31> hop_latency;
}

header int_ingress_port_id_header_t {
    bit<1>  bos;
    bit<15> ingress_port_id_1;
    bit<16> ingress_port_id_0;
}

header int_ingress_tstamp_header_t {
    bit<1>  bos;
    bit<31> ingress_tstamp;
}

header int_q_congestion_header_t {
    bit<1>  bos;
    bit<31> q_congestion;
}

header int_q_occupancy_header_t {
    bit<1>  bos;
    bit<7>  q_occupancy1;
    bit<24> q_occupancy0;
}

header int_switch_id_header_t {
    bit<1>  bos;
    bit<31> switch_id;
}

header lisp_t {
    bit<8>  flags;
    bit<24> nonce;
    bit<32> lsbsInstanceId;
}

header llc_header_t {
    bit<8> dsap;
    bit<8> ssap;
    bit<8> control_;
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
    bit<8>  flow_id;
}

header roce_header_t {
    bit<320> ib_grh;
    bit<96>  ib_bth;
}

header roce_v2_header_t {
    bit<96> ib_bth;
}

header sflow_hdr_t {
    bit<32> version;
    bit<32> addrType;
    bit<32> ipAddress;
    bit<32> subAgentId;
    bit<32> seqNumber;
    bit<32> uptime;
    bit<32> numSamples;
}

header sflow_raw_hdr_record_t {
    bit<20> enterprise;
    bit<12> format;
    bit<32> flowDataLength;
    bit<32> headerProtocol;
    bit<32> frameLength;
    bit<32> bytesRemoved;
    bit<32> headerSize;
}

header sflow_sample_t {
    bit<20> enterprise;
    bit<12> format;
    bit<32> sampleLength;
    bit<32> seqNumer;
    bit<8>  srcIdType;
    bit<24> srcIdIndex;
    bit<32> samplingRate;
    bit<32> samplePool;
    bit<32> numDrops;
    bit<32> inputIfindex;
    bit<32> outputIfindex;
    bit<32> numFlowRecords;
}

header snap_header_t {
    bit<24> oui;
    bit<16> type_;
}

header trill_t {
    bit<2>  version;
    bit<2>  reserved;
    bit<1>  multiDestination;
    bit<5>  optLength;
    bit<6>  hopCount;
    bit<16> egressRbridge;
    bit<16> ingressRbridge;
}

header vntag_t {
    bit<1>  direction;
    bit<1>  pointer;
    bit<14> destVif;
    bit<1>  looped;
    bit<1>  reserved;
    bit<2>  version;
    bit<12> srcVif;
}

header vxlan_t {
    bit<8>  flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8>  reserved2;
}

header vxlan_gpe_t {
    bit<8>  flags;
    bit<16> reserved;
    bit<8>  next_proto;
    bit<24> vni;
    bit<8>  reserved2;
}

header vxlan_gpe_int_header_t {
    bit<8> int_type;
    bit<8> rsvd;
    bit<8> len;
    bit<8> next_proto;
}

header int_value_t {
    bit<1>  bos;
    bit<31> val;
}

header mpls_t {
    bit<20> label;
    bit<3>  exp;
    bit<1>  bos;
    bit<8>  ttl;
}

header vlan_tag_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> etherType;
}

struct metadata {
    @name(".acl_metadata") 
    acl_metadata_t           acl_metadata;
    @name(".egress_filter_metadata") 
    egress_filter_metadata_t egress_filter_metadata;
    @name(".egress_metadata") 
    egress_metadata_t        egress_metadata;
    @name(".fabric_metadata") 
    fabric_metadata_t        fabric_metadata;
    @name(".global_config_metadata") 
    global_config_metadata_t global_config_metadata;
    @name(".hash_metadata") 
    hash_metadata_t          hash_metadata;
    @name(".i2e_metadata") 
    i2e_metadata_t           i2e_metadata;
    @name(".ingress_metadata") 
    ingress_metadata_t       ingress_metadata;
    @name(".int_metadata") 
    int_metadata_t           int_metadata;
    @name(".int_metadata_i2e") 
    int_metadata_i2e_t       int_metadata_i2e;
    @name(".ipv4_metadata") 
    ipv4_metadata_t          ipv4_metadata;
    @name(".ipv6_metadata") 
    ipv6_metadata_t          ipv6_metadata;
    @name(".l2_metadata") 
    l2_metadata_t            l2_metadata;
    @name(".l3_metadata") 
    l3_metadata_t            l3_metadata;
    @name(".meter_metadata") 
    meter_metadata_t         meter_metadata;
    @name(".multicast_metadata") 
    multicast_metadata_t     multicast_metadata;
    @name(".nexthop_metadata") 
    nexthop_metadata_t       nexthop_metadata;
    @name(".qos_metadata") 
    qos_metadata_t           qos_metadata;
    @name(".security_metadata") 
    security_metadata_t      security_metadata;
    @name(".sflow_metadata") 
    sflow_meta_t             sflow_metadata;
    @name(".tunnel_metadata") 
    tunnel_metadata_t        tunnel_metadata;
}

struct headers {
    @name(".arp_rarp") 
    arp_rarp_t                              arp_rarp;
    @name(".arp_rarp_ipv4") 
    arp_rarp_ipv4_t                         arp_rarp_ipv4;
    @name(".bfd") 
    bfd_t                                   bfd;
    @name(".eompls") 
    eompls_t                                eompls;
    @name(".erspan_t3_header") 
    erspan_header_t3_t_0                    erspan_t3_header;
    @name(".ethernet") 
    ethernet_t                              ethernet;
    @name(".fabric_header") 
    fabric_header_t                         fabric_header;
    @name(".fabric_header_cpu") 
    fabric_header_cpu_t                     fabric_header_cpu;
    @name(".fabric_header_mirror") 
    fabric_header_mirror_t                  fabric_header_mirror;
    @name(".fabric_header_multicast") 
    fabric_header_multicast_t               fabric_header_multicast;
    @name(".fabric_header_sflow") 
    fabric_header_sflow_t                   fabric_header_sflow;
    @name(".fabric_header_unicast") 
    fabric_header_unicast_t                 fabric_header_unicast;
    @name(".fabric_payload_header") 
    fabric_payload_header_t                 fabric_payload_header;
    @name(".fcoe") 
    fcoe_header_t                           fcoe;
    @name(".genv") 
    genv_t                                  genv;
    @name(".gre") 
    gre_t                                   gre;
    @name(".icmp") 
    icmp_t                                  icmp;
    @name(".inner_ethernet") 
    ethernet_t                              inner_ethernet;
    @name(".inner_icmp") 
    icmp_t                                  inner_icmp;
    @name(".inner_ipv4") 
    ipv4_t                                  inner_ipv4;
    @name(".inner_ipv6") 
    ipv6_t                                  inner_ipv6;
    @name(".inner_sctp") 
    sctp_t                                  inner_sctp;
    @name(".inner_tcp") 
    tcp_t                                   inner_tcp;
    @name(".inner_udp") 
    udp_t                                   inner_udp;
    @name(".int_egress_port_id_header") 
    int_egress_port_id_header_t             int_egress_port_id_header;
    @name(".int_egress_port_tx_utilization_header") 
    int_egress_port_tx_utilization_header_t int_egress_port_tx_utilization_header;
    @name(".int_header") 
    int_header_t                            int_header;
    @name(".int_hop_latency_header") 
    int_hop_latency_header_t                int_hop_latency_header;
    @name(".int_ingress_port_id_header") 
    int_ingress_port_id_header_t            int_ingress_port_id_header;
    @name(".int_ingress_tstamp_header") 
    int_ingress_tstamp_header_t             int_ingress_tstamp_header;
    @name(".int_q_congestion_header") 
    int_q_congestion_header_t               int_q_congestion_header;
    @name(".int_q_occupancy_header") 
    int_q_occupancy_header_t                int_q_occupancy_header;
    @name(".int_switch_id_header") 
    int_switch_id_header_t                  int_switch_id_header;
    @name(".ipv4") 
    ipv4_t                                  ipv4;
    @name(".ipv6") 
    ipv6_t                                  ipv6;
    @name(".lisp") 
    lisp_t                                  lisp;
    @name(".llc_header") 
    llc_header_t                            llc_header;
    @name(".nsh") 
    nsh_t                                   nsh;
    @name(".nsh_context") 
    nsh_context_t                           nsh_context;
    @name(".nvgre") 
    nvgre_t                                 nvgre;
    @name(".outer_udp") 
    udp_t                                   outer_udp;
    @name(".roce") 
    roce_header_t                           roce;
    @name(".roce_v2") 
    roce_v2_header_t                        roce_v2;
    @name(".sctp") 
    sctp_t                                  sctp;
    @name(".sflow") 
    sflow_hdr_t                             sflow;
    @name(".sflow_raw_hdr_record") 
    sflow_raw_hdr_record_t                  sflow_raw_hdr_record;
    @name(".sflow_sample") 
    sflow_sample_t                          sflow_sample;
    @name(".snap_header") 
    snap_header_t                           snap_header;
    @name(".tcp") 
    tcp_t                                   tcp;
    @name(".trill") 
    trill_t                                 trill;
    @name(".udp") 
    udp_t                                   udp;
    @name(".vntag") 
    vntag_t                                 vntag;
    @name(".vxlan") 
    vxlan_t                                 vxlan;
    @name(".vxlan_gpe") 
    vxlan_gpe_t                             vxlan_gpe;
    @name(".vxlan_gpe_int_header") 
    vxlan_gpe_int_header_t                  vxlan_gpe_int_header;
    @name(".int_val") 
    int_value_t[24]                         int_val;
    @name(".mpls") 
    mpls_t[3]                               mpls;
    @name(".vlan_tag_") 
    vlan_tag_t[2]                           vlan_tag_;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_all_int_meta_value_heders") state parse_all_int_meta_value_heders {
        packet.extract(hdr.int_switch_id_header);
        packet.extract(hdr.int_ingress_port_id_header);
        packet.extract(hdr.int_hop_latency_header);
        packet.extract(hdr.int_q_occupancy_header);
        packet.extract(hdr.int_ingress_tstamp_header);
        packet.extract(hdr.int_egress_port_id_header);
        packet.extract(hdr.int_q_congestion_header);
        packet.extract(hdr.int_egress_port_tx_utilization_header);
        transition parse_int_val;
    }
    @name(".parse_arp_rarp") state parse_arp_rarp {
        packet.extract(hdr.arp_rarp);
        transition select(hdr.arp_rarp.protoType) {
            16w0x800: parse_arp_rarp_ipv4;
            default: accept;
        }
    }
    @name(".parse_arp_rarp_ipv4") state parse_arp_rarp_ipv4 {
        packet.extract(hdr.arp_rarp_ipv4);
        transition parse_set_prio_med;
    }
    @name(".parse_bfd") state parse_bfd {
        packet.extract(hdr.bfd);
        transition parse_set_prio_max;
    }
    @name(".parse_eompls") state parse_eompls {
        meta.tunnel_metadata.ingress_tunnel_type = 5w6;
        transition parse_inner_ethernet;
    }
    @name(".parse_erspan_t3") state parse_erspan_t3 {
        packet.extract(hdr.erspan_t3_header);
        transition parse_inner_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0 &&& 16w0xfe00: parse_llc_header;
            16w0 &&& 16w0xfa00: parse_llc_header;
            16w0x9000: parse_fabric_header;
            16w0x8100: parse_vlan;
            16w0x9100: parse_qinq;
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp_rarp;
            16w0x88cc: parse_set_prio_high;
            16w0x8809: parse_set_prio_high;
            default: accept;
        }
    }
    @name(".parse_fabric_header") state parse_fabric_header {
        packet.extract(hdr.fabric_header);
        transition select(hdr.fabric_header.packetType) {
            3w1: parse_fabric_header_unicast;
            3w2: parse_fabric_header_multicast;
            3w3: parse_fabric_header_mirror;
            3w5: parse_fabric_header_cpu;
            default: accept;
        }
    }
    @name(".parse_fabric_header_cpu") state parse_fabric_header_cpu {
        packet.extract(hdr.fabric_header_cpu);
        meta.ingress_metadata.bypass_lookups = hdr.fabric_header_cpu.reasonCode;
        transition select(hdr.fabric_header_cpu.reasonCode) {
            16w0x4: parse_fabric_sflow_header;
            default: parse_fabric_payload_header;
        }
    }
    @name(".parse_fabric_header_mirror") state parse_fabric_header_mirror {
        packet.extract(hdr.fabric_header_mirror);
        transition parse_fabric_payload_header;
    }
    @name(".parse_fabric_header_multicast") state parse_fabric_header_multicast {
        packet.extract(hdr.fabric_header_multicast);
        transition parse_fabric_payload_header;
    }
    @name(".parse_fabric_header_unicast") state parse_fabric_header_unicast {
        packet.extract(hdr.fabric_header_unicast);
        transition parse_fabric_payload_header;
    }
    @name(".parse_fabric_payload_header") state parse_fabric_payload_header {
        packet.extract(hdr.fabric_payload_header);
        transition select(hdr.fabric_payload_header.etherType) {
            16w0 &&& 16w0xfe00: parse_llc_header;
            16w0 &&& 16w0xfa00: parse_llc_header;
            16w0x8100: parse_vlan;
            16w0x9100: parse_qinq;
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp_rarp;
            16w0x88cc: parse_set_prio_high;
            16w0x8809: parse_set_prio_high;
            default: accept;
        }
    }
    @name(".parse_fabric_sflow_header") state parse_fabric_sflow_header {
        packet.extract(hdr.fabric_header_sflow);
        transition parse_fabric_payload_header;
    }
    @name(".parse_fcoe") state parse_fcoe {
        packet.extract(hdr.fcoe);
        transition accept;
    }
    @name(".parse_geneve") state parse_geneve {
        packet.extract(hdr.genv);
        meta.tunnel_metadata.tunnel_vni = hdr.genv.vni;
        meta.tunnel_metadata.ingress_tunnel_type = 5w4;
        transition select(hdr.genv.ver, hdr.genv.optLen, hdr.genv.protoType) {
            (2w0x0, 6w0x0, 16w0x6558): parse_inner_ethernet;
            (2w0x0, 6w0x0, 16w0x800): parse_inner_ipv4;
            (2w0x0, 6w0x0, 16w0x86dd): parse_inner_ipv6;
            default: accept;
        }
    }
    @name(".parse_gpe_int_header") state parse_gpe_int_header {
        packet.extract(hdr.vxlan_gpe_int_header);
        meta.int_metadata.gpe_int_hdr_len = (bit<16>)hdr.vxlan_gpe_int_header.len;
        transition parse_int_header;
    }
    @name(".parse_gre") state parse_gre {
        packet.extract(hdr.gre);
        transition select(hdr.gre.C, hdr.gre.R, hdr.gre.K, hdr.gre.S, hdr.gre.s, hdr.gre.recurse, hdr.gre.flags, hdr.gre.ver, hdr.gre.proto) {
            (1w0x0, 1w0x0, 1w0x1, 1w0x0, 1w0x0, 3w0x0, 5w0x0, 3w0x0, 16w0x6558): parse_nvgre;
            (1w0x0, 1w0x0, 1w0x0, 1w0x0, 1w0x0, 3w0x0, 5w0x0, 3w0x0, 16w0x800): parse_gre_ipv4;
            (1w0x0, 1w0x0, 1w0x0, 1w0x0, 1w0x0, 3w0x0, 5w0x0, 3w0x0, 16w0x86dd): parse_gre_ipv6;
            (1w0x0, 1w0x0, 1w0x0, 1w0x0, 1w0x0, 3w0x0, 5w0x0, 3w0x0, 16w0x22eb): parse_erspan_t3;
            default: accept;
        }
    }
    @name(".parse_gre_ipv4") state parse_gre_ipv4 {
        meta.tunnel_metadata.ingress_tunnel_type = 5w2;
        transition parse_inner_ipv4;
    }
    @name(".parse_gre_ipv6") state parse_gre_ipv6 {
        meta.tunnel_metadata.ingress_tunnel_type = 5w2;
        transition parse_inner_ipv6;
    }
    @name(".parse_gre_v6") state parse_gre_v6 {
        packet.extract(hdr.gre);
        transition select(hdr.gre.C, hdr.gre.R, hdr.gre.K, hdr.gre.S, hdr.gre.s, hdr.gre.recurse, hdr.gre.flags, hdr.gre.ver, hdr.gre.proto) {
            (1w0x0, 1w0x0, 1w0x0, 1w0x0, 1w0x0, 3w0x0, 5w0x0, 3w0x0, 16w0x800): parse_gre_ipv4;
            default: accept;
        }
    }
    @name(".parse_icmp") state parse_icmp {
        packet.extract(hdr.icmp);
        meta.l3_metadata.lkp_outer_l4_sport = hdr.icmp.typeCode;
        transition select(hdr.icmp.typeCode) {
            16w0x8200 &&& 16w0xfe00: parse_set_prio_med;
            16w0x8400 &&& 16w0xfc00: parse_set_prio_med;
            16w0x8800 &&& 16w0xff00: parse_set_prio_med;
            default: accept;
        }
    }
    @name(".parse_inner_ethernet") state parse_inner_ethernet {
        packet.extract(hdr.inner_ethernet);
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        transition select(hdr.inner_ethernet.etherType) {
            16w0x800: parse_inner_ipv4;
            16w0x86dd: parse_inner_ipv6;
            default: accept;
        }
    }
    @name(".parse_inner_icmp") state parse_inner_icmp {
        packet.extract(hdr.inner_icmp);
        meta.l3_metadata.lkp_l4_sport = hdr.inner_icmp.typeCode;
        transition accept;
    }
    @name(".parse_inner_ipv4") state parse_inner_ipv4 {
        packet.extract(hdr.inner_ipv4);
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.inner_ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.inner_ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv4.protocol;
        meta.l3_metadata.lkp_ip_ttl = hdr.inner_ipv4.ttl;
        transition select(hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ihl, hdr.inner_ipv4.protocol) {
            (13w0x0, 4w0x5, 8w0x1): parse_inner_icmp;
            (13w0x0, 4w0x5, 8w0x6): parse_inner_tcp;
            (13w0x0, 4w0x5, 8w0x11): parse_inner_udp;
            default: accept;
        }
    }
    @name(".parse_inner_ipv6") state parse_inner_ipv6 {
        packet.extract(hdr.inner_ipv6);
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv6.nextHdr;
        meta.l3_metadata.lkp_ip_ttl = hdr.inner_ipv6.hopLimit;
        transition select(hdr.inner_ipv6.nextHdr) {
            8w58: parse_inner_icmp;
            8w6: parse_inner_tcp;
            8w17: parse_inner_udp;
            default: accept;
        }
    }
    @name(".parse_inner_sctp") state parse_inner_sctp {
        packet.extract(hdr.inner_sctp);
        transition accept;
    }
    @name(".parse_inner_tcp") state parse_inner_tcp {
        packet.extract(hdr.inner_tcp);
        meta.l3_metadata.lkp_l4_sport = hdr.inner_tcp.srcPort;
        meta.l3_metadata.lkp_l4_dport = hdr.inner_tcp.dstPort;
        transition accept;
    }
    @name(".parse_inner_udp") state parse_inner_udp {
        packet.extract(hdr.inner_udp);
        meta.l3_metadata.lkp_l4_sport = hdr.inner_udp.srcPort;
        meta.l3_metadata.lkp_l4_dport = hdr.inner_udp.dstPort;
        transition accept;
    }
    @name(".parse_int_header") state parse_int_header {
        packet.extract(hdr.int_header);
        meta.int_metadata.instruction_cnt = (bit<16>)hdr.int_header.ins_cnt;
        transition select(hdr.int_header.rsvd1, hdr.int_header.total_hop_cnt) {
            (5w0x0, 8w0x0): accept;
            (5w0x0 &&& 5w0xf, 8w0x0 &&& 8w0x0): parse_int_val;
            default: accept;
            default: parse_all_int_meta_value_heders;
        }
    }
    @name(".parse_int_val") state parse_int_val {
        packet.extract(hdr.int_val.next);
        transition select(hdr.int_val.last.bos) {
            1w0: parse_int_val;
            1w1: parse_inner_ethernet;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.fragOffset, hdr.ipv4.ihl, hdr.ipv4.protocol) {
            (13w0x0, 4w0x5, 8w0x1): parse_icmp;
            (13w0x0, 4w0x5, 8w0x6): parse_tcp;
            (13w0x0, 4w0x5, 8w0x11): parse_udp;
            (13w0x0, 4w0x5, 8w0x2f): parse_gre;
            (13w0x0, 4w0x5, 8w0x4): parse_ipv4_in_ip;
            (13w0x0, 4w0x5, 8w0x29): parse_ipv6_in_ip;
            (13w0, 4w0, 8w2): parse_set_prio_med;
            (13w0, 4w0, 8w88): parse_set_prio_med;
            (13w0, 4w0, 8w89): parse_set_prio_med;
            (13w0, 4w0, 8w103): parse_set_prio_med;
            (13w0, 4w0, 8w112): parse_set_prio_med;
            default: accept;
        }
    }
    @name(".parse_ipv4_in_ip") state parse_ipv4_in_ip {
        meta.tunnel_metadata.ingress_tunnel_type = 5w3;
        transition parse_inner_ipv4;
    }
    @name(".parse_ipv6") state parse_ipv6 {
        packet.extract(hdr.ipv6);
        transition select(hdr.ipv6.nextHdr) {
            8w58: parse_icmp;
            8w6: parse_tcp;
            8w4: parse_ipv4_in_ip;
            8w17: parse_udp;
            8w47: parse_gre;
            8w41: parse_ipv6_in_ip;
            8w88: parse_set_prio_med;
            8w89: parse_set_prio_med;
            8w103: parse_set_prio_med;
            8w112: parse_set_prio_med;
            default: accept;
        }
    }
    @name(".parse_ipv6_in_ip") state parse_ipv6_in_ip {
        meta.tunnel_metadata.ingress_tunnel_type = 5w3;
        transition parse_inner_ipv6;
    }
    @name(".parse_lisp") state parse_lisp {
        packet.extract(hdr.lisp);
        transition select((packet.lookahead<bit<4>>())[3:0]) {
            4w0x4: parse_inner_ipv4;
            4w0x6: parse_inner_ipv6;
            default: accept;
        }
    }
    @name(".parse_llc_header") state parse_llc_header {
        packet.extract(hdr.llc_header);
        transition select(hdr.llc_header.dsap, hdr.llc_header.ssap) {
            (8w0xaa, 8w0xaa): parse_snap_header;
            (8w0xfe, 8w0xfe): parse_set_prio_med;
            default: accept;
        }
    }
    @name(".parse_mpls") state parse_mpls {
        packet.extract(hdr.mpls.next);
        transition select(hdr.mpls.last.bos) {
            1w0: parse_mpls;
            1w1: parse_mpls_bos;
            default: accept;
        }
    }
    @name(".parse_mpls_bos") state parse_mpls_bos {
        transition select((packet.lookahead<bit<4>>())[3:0]) {
            4w0x4: parse_mpls_inner_ipv4;
            4w0x6: parse_mpls_inner_ipv6;
            default: parse_eompls;
        }
    }
    @name(".parse_mpls_inner_ipv4") state parse_mpls_inner_ipv4 {
        meta.tunnel_metadata.ingress_tunnel_type = 5w9;
        transition parse_inner_ipv4;
    }
    @name(".parse_mpls_inner_ipv6") state parse_mpls_inner_ipv6 {
        meta.tunnel_metadata.ingress_tunnel_type = 5w9;
        transition parse_inner_ipv6;
    }
    @name(".parse_nsh") state parse_nsh {
        packet.extract(hdr.nsh);
        packet.extract(hdr.nsh_context);
        transition select(hdr.nsh.protoType) {
            16w0x800: parse_inner_ipv4;
            16w0x86dd: parse_inner_ipv6;
            16w0x6558: parse_inner_ethernet;
            default: accept;
        }
    }
    @name(".parse_nvgre") state parse_nvgre {
        packet.extract(hdr.nvgre);
        meta.tunnel_metadata.ingress_tunnel_type = 5w5;
        meta.tunnel_metadata.tunnel_vni = hdr.nvgre.tni;
        transition parse_inner_ethernet;
    }
    @name(".parse_pw") state parse_pw {
        transition accept;
    }
    @name(".parse_qinq") state parse_qinq {
        packet.extract(hdr.vlan_tag_[0]);
        transition select(hdr.vlan_tag_[0].etherType) {
            16w0x8100: parse_qinq_vlan;
            default: accept;
        }
    }
    @name(".parse_qinq_vlan") state parse_qinq_vlan {
        packet.extract(hdr.vlan_tag_[1]);
        transition select(hdr.vlan_tag_[1].etherType) {
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp_rarp;
            16w0x88cc: parse_set_prio_high;
            16w0x8809: parse_set_prio_high;
            default: accept;
        }
    }
    @name(".parse_roce") state parse_roce {
        packet.extract(hdr.roce);
        transition accept;
    }
    @name(".parse_roce_v2") state parse_roce_v2 {
        packet.extract(hdr.roce_v2);
        transition accept;
    }
    @name(".parse_sctp") state parse_sctp {
        packet.extract(hdr.sctp);
        transition accept;
    }
    @name(".parse_set_prio_high") state parse_set_prio_high {
        standard_metadata.priority = 3w5;
        transition accept;
    }
    @name(".parse_set_prio_max") state parse_set_prio_max {
        standard_metadata.priority = 3w7;
        transition accept;
    }
    @name(".parse_set_prio_med") state parse_set_prio_med {
        standard_metadata.priority = 3w3;
        transition accept;
    }
    @name(".parse_sflow") state parse_sflow {
        packet.extract(hdr.sflow);
        transition accept;
    }
    @name(".parse_snap_header") state parse_snap_header {
        packet.extract(hdr.snap_header);
        transition select(hdr.snap_header.type_) {
            16w0x8100: parse_vlan;
            16w0x9100: parse_qinq;
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp_rarp;
            16w0x88cc: parse_set_prio_high;
            16w0x8809: parse_set_prio_high;
            default: accept;
        }
    }
    @name(".parse_tcp") state parse_tcp {
        packet.extract(hdr.tcp);
        meta.l3_metadata.lkp_outer_l4_sport = hdr.tcp.srcPort;
        meta.l3_metadata.lkp_outer_l4_dport = hdr.tcp.dstPort;
        transition select(hdr.tcp.dstPort) {
            16w179: parse_set_prio_med;
            16w639: parse_set_prio_med;
            default: accept;
        }
    }
    @name(".parse_trill") state parse_trill {
        packet.extract(hdr.trill);
        transition parse_inner_ethernet;
    }
    @name(".parse_udp") state parse_udp {
        packet.extract(hdr.udp);
        meta.l3_metadata.lkp_outer_l4_sport = hdr.udp.srcPort;
        meta.l3_metadata.lkp_outer_l4_dport = hdr.udp.dstPort;
        transition select(hdr.udp.dstPort) {
            16w4789: parse_vxlan;
            16w6081: parse_geneve;
            16w4790: parse_vxlan_gpe;
            16w67: parse_set_prio_med;
            16w68: parse_set_prio_med;
            16w546: parse_set_prio_med;
            16w547: parse_set_prio_med;
            16w520: parse_set_prio_med;
            16w521: parse_set_prio_med;
            16w1985: parse_set_prio_med;
            16w6343: parse_sflow;
            default: accept;
        }
    }
    @name(".parse_udp_v6") state parse_udp_v6 {
        packet.extract(hdr.udp);
        meta.l3_metadata.lkp_outer_l4_sport = hdr.udp.srcPort;
        meta.l3_metadata.lkp_outer_l4_dport = hdr.udp.dstPort;
        transition select(hdr.udp.dstPort) {
            16w67: parse_set_prio_med;
            16w68: parse_set_prio_med;
            16w546: parse_set_prio_med;
            16w547: parse_set_prio_med;
            16w520: parse_set_prio_med;
            16w521: parse_set_prio_med;
            16w1985: parse_set_prio_med;
            default: accept;
        }
    }
    @name(".parse_vlan") state parse_vlan {
        packet.extract(hdr.vlan_tag_[0]);
        transition select(hdr.vlan_tag_[0].etherType) {
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp_rarp;
            16w0x88cc: parse_set_prio_high;
            16w0x8809: parse_set_prio_high;
            default: accept;
        }
    }
    @name(".parse_vntag") state parse_vntag {
        packet.extract(hdr.vntag);
        transition parse_inner_ethernet;
    }
    @name(".parse_vpls") state parse_vpls {
        transition accept;
    }
    @name(".parse_vxlan") state parse_vxlan {
        packet.extract(hdr.vxlan);
        meta.tunnel_metadata.ingress_tunnel_type = 5w1;
        meta.tunnel_metadata.tunnel_vni = hdr.vxlan.vni;
        transition parse_inner_ethernet;
    }
    @name(".parse_vxlan_gpe") state parse_vxlan_gpe {
        packet.extract(hdr.vxlan_gpe);
        meta.tunnel_metadata.ingress_tunnel_type = 5w12;
        meta.tunnel_metadata.tunnel_vni = hdr.vxlan_gpe.vni;
        transition select(hdr.vxlan_gpe.flags, hdr.vxlan_gpe.next_proto) {
            (8w0x8 &&& 8w0x8, 8w0x5 &&& 8w0xff): parse_gpe_int_header;
            default: parse_inner_ethernet;
        }
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

@name(".bd_action_profile") action_profile(32w1024) bd_action_profile;

@name(".ecmp_action_profile") @mode("fair") action_selector(HashAlgorithm.identity, 32w1024, 32w10) ecmp_action_profile;

@name(".fabric_lag_action_profile") @mode("fair") action_selector(HashAlgorithm.identity, 32w1024, 32w8) fabric_lag_action_profile;

@name(".lag_action_profile") @mode("fair") action_selector(HashAlgorithm.identity, 32w1024, 32w8) lag_action_profile;

control process_replication(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_replica_copy_bridged") action set_replica_copy_bridged() {
        meta.egress_metadata.routed = 1w0;
    }
    @name(".outer_replica_from_rid") action outer_replica_from_rid(bit<16> bd, bit<14> tunnel_index, bit<5> tunnel_type, bit<4> header_count) {
        meta.egress_metadata.bd = bd;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w0;
        meta.egress_metadata.routed = meta.l3_metadata.outer_routed;
        meta.egress_metadata.same_bd_check = bd ^ meta.ingress_metadata.outer_bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
        meta.tunnel_metadata.egress_header_count = header_count;
    }
    @name(".inner_replica_from_rid") action inner_replica_from_rid(bit<16> bd, bit<14> tunnel_index, bit<5> tunnel_type, bit<4> header_count) {
        meta.egress_metadata.bd = bd;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w1;
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.same_bd_check = bd ^ meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
        meta.tunnel_metadata.egress_header_count = header_count;
    }
    @name(".replica_type") table replica_type {
        actions = {
            nop;
            set_replica_copy_bridged;
        }
        key = {
            meta.multicast_metadata.replica   : exact;
            meta.egress_metadata.same_bd_check: ternary;
        }
        size = 512;
    }
    @name(".rid") table rid {
        actions = {
            nop;
            outer_replica_from_rid;
            inner_replica_from_rid;
        }
        key = {
            standard_metadata.egress_rid: exact;
        }
        size = 1024;
    }
    apply {
        if (standard_metadata.egress_rid != 16w0) {
            rid.apply();
            replica_type.apply();
        }
    }
}

control process_vlan_decap(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".remove_vlan_single_tagged") action remove_vlan_single_tagged() {
        hdr.ethernet.etherType = hdr.vlan_tag_[0].etherType;
        hdr.vlan_tag_[0].setInvalid();
    }
    @name(".remove_vlan_double_tagged") action remove_vlan_double_tagged() {
        hdr.ethernet.etherType = hdr.vlan_tag_[1].etherType;
        hdr.vlan_tag_[0].setInvalid();
        hdr.vlan_tag_[1].setInvalid();
    }
    @name(".vlan_decap") table vlan_decap {
        actions = {
            nop;
            remove_vlan_single_tagged;
            remove_vlan_double_tagged;
        }
        key = {
            hdr.vlan_tag_[0].isValid(): exact;
            hdr.vlan_tag_[1].isValid(): exact;
        }
        size = 1024;
    }
    apply {
        vlan_decap.apply();
    }
}

control process_tunnel_decap(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".decap_inner_udp") action decap_inner_udp() {
        hdr.udp = hdr.inner_udp;
        hdr.inner_udp.setInvalid();
    }
    @name(".decap_inner_tcp") action decap_inner_tcp() {
        hdr.tcp = hdr.inner_tcp;
        hdr.inner_tcp.setInvalid();
        hdr.udp.setInvalid();
    }
    @name(".decap_inner_icmp") action decap_inner_icmp() {
        hdr.icmp = hdr.inner_icmp;
        hdr.inner_icmp.setInvalid();
        hdr.udp.setInvalid();
    }
    @name(".decap_inner_unknown") action decap_inner_unknown() {
        hdr.udp.setInvalid();
    }
    @name(".decap_vxlan_inner_ipv4") action decap_vxlan_inner_ipv4() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.vxlan.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_vxlan_inner_ipv6") action decap_vxlan_inner_ipv6() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.vxlan.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_vxlan_inner_non_ip") action decap_vxlan_inner_non_ip() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.vxlan.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_genv_inner_ipv4") action decap_genv_inner_ipv4() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.genv.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_genv_inner_ipv6") action decap_genv_inner_ipv6() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.genv.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_genv_inner_non_ip") action decap_genv_inner_non_ip() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.genv.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_nvgre_inner_ipv4") action decap_nvgre_inner_ipv4() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_nvgre_inner_ipv6") action decap_nvgre_inner_ipv6() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_nvgre_inner_non_ip") action decap_nvgre_inner_non_ip() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_gre_inner_ipv4") action decap_gre_inner_ipv4() {
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.gre.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_gre_inner_ipv6") action decap_gre_inner_ipv6() {
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_gre_inner_non_ip") action decap_gre_inner_non_ip() {
        hdr.ethernet.etherType = hdr.gre.proto;
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_ip_inner_ipv4") action decap_ip_inner_ipv4() {
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.ipv6.setInvalid();
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_ip_inner_ipv6") action decap_ip_inner_ipv6() {
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.ipv4.setInvalid();
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ipv4_pop1") action decap_mpls_inner_ipv4_pop1() {
        hdr.mpls[0].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop1") action decap_mpls_inner_ipv6_pop1() {
        hdr.mpls[0].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop1") action decap_mpls_inner_ethernet_ipv4_pop1() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop1") action decap_mpls_inner_ethernet_ipv6_pop1() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop1") action decap_mpls_inner_ethernet_non_ip_pop1() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".decap_mpls_inner_ipv4_pop2") action decap_mpls_inner_ipv4_pop2() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop2") action decap_mpls_inner_ipv6_pop2() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop2") action decap_mpls_inner_ethernet_ipv4_pop2() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop2") action decap_mpls_inner_ethernet_ipv6_pop2() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop2") action decap_mpls_inner_ethernet_non_ip_pop2() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".decap_mpls_inner_ipv4_pop3") action decap_mpls_inner_ipv4_pop3() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop3") action decap_mpls_inner_ipv6_pop3() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop3") action decap_mpls_inner_ethernet_ipv4_pop3() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop3") action decap_mpls_inner_ethernet_ipv6_pop3() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop3") action decap_mpls_inner_ethernet_non_ip_pop3() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".tunnel_decap_process_inner") table tunnel_decap_process_inner {
        actions = {
            decap_inner_udp;
            decap_inner_tcp;
            decap_inner_icmp;
            decap_inner_unknown;
        }
        key = {
            hdr.inner_tcp.isValid() : exact;
            hdr.inner_udp.isValid() : exact;
            hdr.inner_icmp.isValid(): exact;
        }
        size = 1024;
    }
    @name(".tunnel_decap_process_outer") table tunnel_decap_process_outer {
        actions = {
            decap_vxlan_inner_ipv4;
            decap_vxlan_inner_ipv6;
            decap_vxlan_inner_non_ip;
            decap_genv_inner_ipv4;
            decap_genv_inner_ipv6;
            decap_genv_inner_non_ip;
            decap_nvgre_inner_ipv4;
            decap_nvgre_inner_ipv6;
            decap_nvgre_inner_non_ip;
            decap_gre_inner_ipv4;
            decap_gre_inner_ipv6;
            decap_gre_inner_non_ip;
            decap_ip_inner_ipv4;
            decap_ip_inner_ipv6;
            decap_mpls_inner_ipv4_pop1;
            decap_mpls_inner_ipv6_pop1;
            decap_mpls_inner_ethernet_ipv4_pop1;
            decap_mpls_inner_ethernet_ipv6_pop1;
            decap_mpls_inner_ethernet_non_ip_pop1;
            decap_mpls_inner_ipv4_pop2;
            decap_mpls_inner_ipv6_pop2;
            decap_mpls_inner_ethernet_ipv4_pop2;
            decap_mpls_inner_ethernet_ipv6_pop2;
            decap_mpls_inner_ethernet_non_ip_pop2;
            decap_mpls_inner_ipv4_pop3;
            decap_mpls_inner_ipv6_pop3;
            decap_mpls_inner_ethernet_ipv4_pop3;
            decap_mpls_inner_ethernet_ipv6_pop3;
            decap_mpls_inner_ethernet_non_ip_pop3;
        }
        key = {
            meta.tunnel_metadata.ingress_tunnel_type: exact;
            hdr.inner_ipv4.isValid()                : exact;
            hdr.inner_ipv6.isValid()                : exact;
        }
        size = 1024;
    }
    apply {
        if (meta.tunnel_metadata.tunnel_terminate == 1w1) {
            if (meta.multicast_metadata.inner_replica == 1w1 || meta.multicast_metadata.replica == 1w0) {
                tunnel_decap_process_outer.apply();
                tunnel_decap_process_inner.apply();
            }
        }
    }
}

control process_rewrite(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_l2_rewrite") action set_l2_rewrite() {
        meta.egress_metadata.routed = 1w0;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.egress_metadata.outer_bd = meta.ingress_metadata.bd;
    }
    @name(".set_l2_rewrite_with_tunnel") action set_l2_rewrite_with_tunnel(bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.routed = 1w0;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.egress_metadata.outer_bd = meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".set_l3_rewrite") action set_l3_rewrite(bit<16> bd, bit<8> mtu_index, bit<48> dmac) {
        meta.egress_metadata.routed = 1w1;
        meta.egress_metadata.mac_da = dmac;
        meta.egress_metadata.bd = bd;
        meta.egress_metadata.outer_bd = bd;
        meta.l3_metadata.mtu_index = mtu_index;
    }
    @name(".set_l3_rewrite_with_tunnel") action set_l3_rewrite_with_tunnel(bit<16> bd, bit<48> dmac, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.routed = 1w1;
        meta.egress_metadata.mac_da = dmac;
        meta.egress_metadata.bd = bd;
        meta.egress_metadata.outer_bd = bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".set_mpls_swap_push_rewrite_l2") action set_mpls_swap_push_rewrite_l2(bit<20> label, bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        hdr.mpls[0].label = label;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w13;
    }
    @name(".set_mpls_push_rewrite_l2") action set_mpls_push_rewrite_l2(bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w13;
    }
    @name(".set_mpls_swap_push_rewrite_l3") action set_mpls_swap_push_rewrite_l3(bit<16> bd, bit<48> dmac, bit<20> label, bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = bd;
        hdr.mpls[0].label = label;
        meta.egress_metadata.mac_da = dmac;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w14;
    }
    @name(".set_mpls_push_rewrite_l3") action set_mpls_push_rewrite_l3(bit<16> bd, bit<48> dmac, bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = bd;
        meta.egress_metadata.mac_da = dmac;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w14;
    }
    @name(".rewrite_ipv4_multicast") action rewrite_ipv4_multicast() {
        hdr.ethernet.dstAddr[22:0] = ((bit<48>)hdr.ipv4.dstAddr)[22:0];
    }
    @name(".rewrite_ipv6_multicast") action rewrite_ipv6_multicast() {
    }
    @name(".rewrite") table rewrite {
        actions = {
            nop;
            set_l2_rewrite;
            set_l2_rewrite_with_tunnel;
            set_l3_rewrite;
            set_l3_rewrite_with_tunnel;
            set_mpls_swap_push_rewrite_l2;
            set_mpls_push_rewrite_l2;
            set_mpls_swap_push_rewrite_l3;
            set_mpls_push_rewrite_l3;
        }
        key = {
            meta.l3_metadata.nexthop_index: exact;
        }
        size = 1024;
    }
    @name(".rewrite_multicast") table rewrite_multicast {
        actions = {
            nop;
            rewrite_ipv4_multicast;
            rewrite_ipv6_multicast;
        }
        key = {
            hdr.ipv4.isValid()       : exact;
            hdr.ipv6.isValid()       : exact;
            hdr.ipv4.dstAddr[31:28]  : ternary @name("ipv4.dstAddr") ;
            hdr.ipv6.dstAddr[127:120]: ternary @name("ipv6.dstAddr") ;
        }
    }
    apply {
        if (meta.egress_metadata.routed == 1w0 || meta.l3_metadata.nexthop_index != 16w0) {
            rewrite.apply();
        } else {
            rewrite_multicast.apply();
        }
    }
}

control process_egress_bd(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_egress_bd_properties") action set_egress_bd_properties(bit<9> smac_idx) {
        meta.egress_metadata.smac_idx = smac_idx;
    }
    @name(".egress_bd_map") table egress_bd_map {
        actions = {
            nop;
            set_egress_bd_properties;
        }
        key = {
            meta.egress_metadata.bd: exact;
        }
        size = 1024;
    }
    apply {
        egress_bd_map.apply();
    }
}

control process_mac_rewrite(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".ipv4_unicast_rewrite") action ipv4_unicast_rewrite() {
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 8w1;
    }
    @name(".ipv4_multicast_rewrite") action ipv4_multicast_rewrite() {
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | 48w0x1005e000000;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 8w1;
    }
    @name(".ipv6_unicast_rewrite") action ipv6_unicast_rewrite() {
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit - 8w1;
    }
    @name(".ipv6_multicast_rewrite") action ipv6_multicast_rewrite() {
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | 48w0x333300000000;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit - 8w1;
    }
    @name(".mpls_rewrite") action mpls_rewrite() {
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.mpls[0].ttl = hdr.mpls[0].ttl - 8w1;
    }
    @name(".rewrite_smac") action rewrite_smac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name(".l3_rewrite") table l3_rewrite {
        actions = {
            nop;
            ipv4_unicast_rewrite;
            ipv4_multicast_rewrite;
            ipv6_unicast_rewrite;
            ipv6_multicast_rewrite;
            mpls_rewrite;
        }
        key = {
            hdr.ipv4.isValid()       : exact;
            hdr.ipv6.isValid()       : exact;
            hdr.mpls[0].isValid()    : exact;
            hdr.ipv4.dstAddr[31:28]  : ternary @name("ipv4.dstAddr") ;
            hdr.ipv6.dstAddr[127:120]: ternary @name("ipv6.dstAddr") ;
        }
    }
    @name(".smac_rewrite") table smac_rewrite {
        actions = {
            rewrite_smac;
        }
        key = {
            meta.egress_metadata.smac_idx: exact;
        }
        size = 512;
    }
    apply {
        if (meta.egress_metadata.routed == 1w1) {
            l3_rewrite.apply();
            smac_rewrite.apply();
        }
    }
}

control process_mtu(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".mtu_miss") action mtu_miss() {
        meta.l3_metadata.l3_mtu_check = 16w0xffff;
    }
    @name(".ipv4_mtu_check") action ipv4_mtu_check(bit<16> l3_mtu) {
        meta.l3_metadata.l3_mtu_check = l3_mtu |-| hdr.ipv4.totalLen;
    }
    @name(".ipv6_mtu_check") action ipv6_mtu_check(bit<16> l3_mtu) {
        meta.l3_metadata.l3_mtu_check = l3_mtu |-| hdr.ipv6.payloadLen;
    }
    @name(".mtu") table mtu {
        actions = {
            mtu_miss;
            ipv4_mtu_check;
            ipv6_mtu_check;
        }
        key = {
            meta.l3_metadata.mtu_index: exact;
            hdr.ipv4.isValid()        : exact;
            hdr.ipv6.isValid()        : exact;
        }
        size = 1024;
    }
    apply {
        mtu.apply();
    }
}

control process_int_insertion(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".int_set_header_0_bos") action int_set_header_0_bos() {
        hdr.int_switch_id_header.bos = 1w1;
    }
    @name(".int_set_header_1_bos") action int_set_header_1_bos() {
        hdr.int_ingress_port_id_header.bos = 1w1;
    }
    @name(".int_set_header_2_bos") action int_set_header_2_bos() {
        hdr.int_hop_latency_header.bos = 1w1;
    }
    @name(".int_set_header_3_bos") action int_set_header_3_bos() {
        hdr.int_q_occupancy_header.bos = 1w1;
    }
    @name(".int_set_header_4_bos") action int_set_header_4_bos() {
        hdr.int_ingress_tstamp_header.bos = 1w1;
    }
    @name(".int_set_header_5_bos") action int_set_header_5_bos() {
        hdr.int_egress_port_id_header.bos = 1w1;
    }
    @name(".int_set_header_6_bos") action int_set_header_6_bos() {
        hdr.int_q_congestion_header.bos = 1w1;
    }
    @name(".int_set_header_7_bos") action int_set_header_7_bos() {
        hdr.int_egress_port_tx_utilization_header.bos = 1w1;
    }
    @name(".nop") action nop() {
    }
    @name(".int_transit") action int_transit(bit<32> switch_id) {
        meta.int_metadata.insert_cnt = hdr.int_header.max_hop_cnt - hdr.int_header.total_hop_cnt;
        meta.int_metadata.switch_id = switch_id;
        meta.int_metadata.insert_byte_cnt = meta.int_metadata.instruction_cnt << 2;
        meta.int_metadata.gpe_int_hdr_len8 = (bit<8>)hdr.int_header.ins_cnt;
    }
    @name(".int_src") action int_src(bit<32> switch_id, bit<8> hop_cnt, bit<5> ins_cnt, bit<4> ins_mask0003, bit<4> ins_mask0407, bit<16> ins_byte_cnt, bit<8> total_words) {
        meta.int_metadata.insert_cnt = hop_cnt;
        meta.int_metadata.switch_id = switch_id;
        meta.int_metadata.insert_byte_cnt = ins_byte_cnt;
        meta.int_metadata.gpe_int_hdr_len8 = total_words;
        hdr.int_header.setValid();
        hdr.int_header.ver = 2w0;
        hdr.int_header.rep = 2w0;
        hdr.int_header.c = 1w0;
        hdr.int_header.e = 1w0;
        hdr.int_header.rsvd1 = 5w0;
        hdr.int_header.ins_cnt = ins_cnt;
        hdr.int_header.max_hop_cnt = hop_cnt;
        hdr.int_header.total_hop_cnt = 8w0;
        hdr.int_header.instruction_mask_0003 = ins_mask0003;
        hdr.int_header.instruction_mask_0407 = ins_mask0407;
        hdr.int_header.instruction_mask_0811 = 4w0;
        hdr.int_header.instruction_mask_1215 = 4w0;
        hdr.int_header.rsvd2 = 16w0;
    }
    @name(".int_reset") action int_reset() {
        meta.int_metadata.switch_id = 32w0;
        meta.int_metadata.insert_byte_cnt = 16w0;
        meta.int_metadata.insert_cnt = 8w0;
        meta.int_metadata.gpe_int_hdr_len8 = 8w0;
        meta.int_metadata.gpe_int_hdr_len = 16w0;
        meta.int_metadata.instruction_cnt = 16w0;
    }
    @name(".int_set_header_0003_i0") action int_set_header_0003_i0() {
    }
    @name(".int_set_header_3") action int_set_header_3() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy1 = 7w0;
        hdr.int_q_occupancy_header.q_occupancy0 = (bit<24>)standard_metadata.enq_qdepth;
    }
    @name(".int_set_header_0003_i1") action int_set_header_0003_i1() {
        int_set_header_3();
    }
    @name(".int_set_header_2") action int_set_header_2() {
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)standard_metadata.deq_timedelta;
    }
    @name(".int_set_header_0003_i2") action int_set_header_0003_i2() {
        int_set_header_2();
    }
    @name(".int_set_header_0003_i3") action int_set_header_0003_i3() {
        int_set_header_3();
        int_set_header_2();
    }
    @name(".int_set_header_1") action int_set_header_1() {
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id_1 = 15w0;
        hdr.int_ingress_port_id_header.ingress_port_id_0 = meta.ingress_metadata.ifindex;
    }
    @name(".int_set_header_0003_i4") action int_set_header_0003_i4() {
        int_set_header_1();
    }
    @name(".int_set_header_0003_i5") action int_set_header_0003_i5() {
        int_set_header_3();
        int_set_header_1();
    }
    @name(".int_set_header_0003_i6") action int_set_header_0003_i6() {
        int_set_header_2();
        int_set_header_1();
    }
    @name(".int_set_header_0003_i7") action int_set_header_0003_i7() {
        int_set_header_3();
        int_set_header_2();
        int_set_header_1();
    }
    @name(".int_set_header_0") action int_set_header_0() {
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i8") action int_set_header_0003_i8() {
        int_set_header_0();
    }
    @name(".int_set_header_0003_i9") action int_set_header_0003_i9() {
        int_set_header_3();
        int_set_header_0();
    }
    @name(".int_set_header_0003_i10") action int_set_header_0003_i10() {
        int_set_header_2();
        int_set_header_0();
    }
    @name(".int_set_header_0003_i11") action int_set_header_0003_i11() {
        int_set_header_3();
        int_set_header_2();
        int_set_header_0();
    }
    @name(".int_set_header_0003_i12") action int_set_header_0003_i12() {
        int_set_header_1();
        int_set_header_0();
    }
    @name(".int_set_header_0003_i13") action int_set_header_0003_i13() {
        int_set_header_3();
        int_set_header_1();
        int_set_header_0();
    }
    @name(".int_set_header_0003_i14") action int_set_header_0003_i14() {
        int_set_header_2();
        int_set_header_1();
        int_set_header_0();
    }
    @name(".int_set_header_0003_i15") action int_set_header_0003_i15() {
        int_set_header_3();
        int_set_header_2();
        int_set_header_1();
        int_set_header_0();
    }
    @name(".int_set_header_0407_i0") action int_set_header_0407_i0() {
    }
    @name(".int_set_header_7") action int_set_header_7() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
    }
    @name(".int_set_header_0407_i1") action int_set_header_0407_i1() {
        int_set_header_7();
    }
    @name(".int_set_header_6") action int_set_header_6() {
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
    }
    @name(".int_set_header_0407_i2") action int_set_header_0407_i2() {
        int_set_header_6();
    }
    @name(".int_set_header_0407_i3") action int_set_header_0407_i3() {
        int_set_header_7();
        int_set_header_6();
    }
    @name(".int_set_header_5") action int_set_header_5() {
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
    }
    @name(".int_set_header_0407_i4") action int_set_header_0407_i4() {
        int_set_header_5();
    }
    @name(".int_set_header_0407_i5") action int_set_header_0407_i5() {
        int_set_header_7();
        int_set_header_5();
    }
    @name(".int_set_header_0407_i6") action int_set_header_0407_i6() {
        int_set_header_6();
        int_set_header_5();
    }
    @name(".int_set_header_0407_i7") action int_set_header_0407_i7() {
        int_set_header_7();
        int_set_header_6();
        int_set_header_5();
    }
    @name(".int_set_header_4") action int_set_header_4() {
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i8") action int_set_header_0407_i8() {
        int_set_header_4();
    }
    @name(".int_set_header_0407_i9") action int_set_header_0407_i9() {
        int_set_header_7();
        int_set_header_4();
    }
    @name(".int_set_header_0407_i10") action int_set_header_0407_i10() {
        int_set_header_6();
        int_set_header_4();
    }
    @name(".int_set_header_0407_i11") action int_set_header_0407_i11() {
        int_set_header_7();
        int_set_header_6();
        int_set_header_4();
    }
    @name(".int_set_header_0407_i12") action int_set_header_0407_i12() {
        int_set_header_5();
        int_set_header_4();
    }
    @name(".int_set_header_0407_i13") action int_set_header_0407_i13() {
        int_set_header_7();
        int_set_header_5();
        int_set_header_4();
    }
    @name(".int_set_header_0407_i14") action int_set_header_0407_i14() {
        int_set_header_6();
        int_set_header_5();
        int_set_header_4();
    }
    @name(".int_set_header_0407_i15") action int_set_header_0407_i15() {
        int_set_header_7();
        int_set_header_6();
        int_set_header_5();
        int_set_header_4();
    }
    @name(".int_set_e_bit") action int_set_e_bit() {
        hdr.int_header.e = 1w1;
    }
    @name(".int_update_total_hop_cnt") action int_update_total_hop_cnt() {
        hdr.int_header.total_hop_cnt = hdr.int_header.total_hop_cnt + 8w1;
    }
    @name(".int_bos") table int_bos {
        actions = {
            int_set_header_0_bos;
            int_set_header_1_bos;
            int_set_header_2_bos;
            int_set_header_3_bos;
            int_set_header_4_bos;
            int_set_header_5_bos;
            int_set_header_6_bos;
            int_set_header_7_bos;
            nop;
        }
        key = {
            hdr.int_header.total_hop_cnt        : ternary;
            hdr.int_header.instruction_mask_0003: ternary;
            hdr.int_header.instruction_mask_0407: ternary;
            hdr.int_header.instruction_mask_0811: ternary;
            hdr.int_header.instruction_mask_1215: ternary;
        }
        size = 17;
    }
    @name(".int_insert") table int_insert {
        actions = {
            int_transit;
            int_src;
            int_reset;
        }
        key = {
            meta.int_metadata_i2e.source: ternary;
            meta.int_metadata_i2e.sink  : ternary;
            hdr.int_header.isValid()    : exact;
        }
        size = 3;
    }
    @name(".int_inst_0003") table int_inst_0003 {
        actions = {
            int_set_header_0003_i0;
            int_set_header_0003_i1;
            int_set_header_0003_i2;
            int_set_header_0003_i3;
            int_set_header_0003_i4;
            int_set_header_0003_i5;
            int_set_header_0003_i6;
            int_set_header_0003_i7;
            int_set_header_0003_i8;
            int_set_header_0003_i9;
            int_set_header_0003_i10;
            int_set_header_0003_i11;
            int_set_header_0003_i12;
            int_set_header_0003_i13;
            int_set_header_0003_i14;
            int_set_header_0003_i15;
        }
        key = {
            hdr.int_header.instruction_mask_0003: exact;
        }
        size = 17;
    }
    @name(".int_inst_0407") table int_inst_0407 {
        actions = {
            int_set_header_0407_i0;
            int_set_header_0407_i1;
            int_set_header_0407_i2;
            int_set_header_0407_i3;
            int_set_header_0407_i4;
            int_set_header_0407_i5;
            int_set_header_0407_i6;
            int_set_header_0407_i7;
            int_set_header_0407_i8;
            int_set_header_0407_i9;
            int_set_header_0407_i10;
            int_set_header_0407_i11;
            int_set_header_0407_i12;
            int_set_header_0407_i13;
            int_set_header_0407_i14;
            int_set_header_0407_i15;
            nop;
        }
        key = {
            hdr.int_header.instruction_mask_0407: exact;
        }
        size = 17;
    }
    @name(".int_inst_0811") table int_inst_0811 {
        actions = {
            nop;
        }
        key = {
            hdr.int_header.instruction_mask_0811: exact;
        }
        size = 16;
    }
    @name(".int_inst_1215") table int_inst_1215 {
        actions = {
            nop;
        }
        key = {
            hdr.int_header.instruction_mask_1215: exact;
        }
        size = 17;
    }
    @name(".int_meta_header_update") table int_meta_header_update {
        actions = {
            int_set_e_bit;
            int_update_total_hop_cnt;
        }
        key = {
            meta.int_metadata.insert_cnt: ternary;
        }
        size = 2;
    }
    apply {
        switch (int_insert.apply().action_run) {
            int_transit: {
                if (meta.int_metadata.insert_cnt != 8w0) {
                    int_inst_0003.apply();
                    int_inst_0407.apply();
                    int_inst_0811.apply();
                    int_inst_1215.apply();
                    int_bos.apply();
                }
                int_meta_header_update.apply();
            }
        }

    }
}

control process_egress_bd_stats(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".egress_bd_stats") @min_width(32) direct_counter(CounterType.packets_and_bytes) egress_bd_stats;
    @name(".nop") action nop() {
    }
    @name(".nop") action nop_0() {
        egress_bd_stats.count();
    }
    @name(".egress_bd_stats") table egress_bd_stats_0 {
        actions = {
            nop_0;
        }
        key = {
            meta.egress_metadata.bd      : exact;
            meta.l2_metadata.lkp_pkt_type: exact;
        }
        size = 1024;
        counters = egress_bd_stats;
    }
    apply {
        egress_bd_stats_0.apply();
    }
}

control process_tunnel_encap(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_egress_tunnel_vni") action set_egress_tunnel_vni(bit<24> vnid) {
        meta.tunnel_metadata.vnid = vnid;
    }
    @name(".rewrite_tunnel_dmac") action rewrite_tunnel_dmac(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name(".rewrite_tunnel_ipv4_dst") action rewrite_tunnel_ipv4_dst(bit<32> ip) {
        hdr.ipv4.dstAddr = ip;
    }
    @name(".rewrite_tunnel_ipv6_dst") action rewrite_tunnel_ipv6_dst(bit<128> ip) {
        hdr.ipv6.dstAddr = ip;
    }
    @name(".inner_ipv4_udp_rewrite") action inner_ipv4_udp_rewrite() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_udp = hdr.udp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.udp.setInvalid();
        hdr.ipv4.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w4;
    }
    @name(".inner_ipv4_tcp_rewrite") action inner_ipv4_tcp_rewrite() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_tcp = hdr.tcp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.tcp.setInvalid();
        hdr.ipv4.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w4;
    }
    @name(".inner_ipv4_icmp_rewrite") action inner_ipv4_icmp_rewrite() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_icmp = hdr.icmp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.icmp.setInvalid();
        hdr.ipv4.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w4;
    }
    @name(".inner_ipv4_unknown_rewrite") action inner_ipv4_unknown_rewrite() {
        hdr.inner_ipv4 = hdr.ipv4;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.ipv4.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w4;
    }
    @name(".inner_ipv6_udp_rewrite") action inner_ipv6_udp_rewrite() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_udp = hdr.udp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.ipv6.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w41;
    }
    @name(".inner_ipv6_tcp_rewrite") action inner_ipv6_tcp_rewrite() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_tcp = hdr.tcp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.tcp.setInvalid();
        hdr.ipv6.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w41;
    }
    @name(".inner_ipv6_icmp_rewrite") action inner_ipv6_icmp_rewrite() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_icmp = hdr.icmp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.icmp.setInvalid();
        hdr.ipv6.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w41;
    }
    @name(".inner_ipv6_unknown_rewrite") action inner_ipv6_unknown_rewrite() {
        hdr.inner_ipv6 = hdr.ipv6;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.ipv6.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w41;
    }
    @name(".inner_non_ip_rewrite") action inner_non_ip_rewrite() {
        meta.egress_metadata.payload_length = (bit<16>)standard_metadata.packet_length + 16w65522;
    }
    @name(".f_insert_vxlan_header") action f_insert_vxlan_header() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.udp.setValid();
        hdr.vxlan.setValid();
        hdr.udp.srcPort = meta.hash_metadata.entropy_hash;
        hdr.udp.dstPort = 16w4789;
        hdr.udp.checksum = 16w0;
        hdr.udp.length_ = meta.egress_metadata.payload_length + 16w30;
        hdr.vxlan.flags = 8w0x8;
        hdr.vxlan.reserved = 24w0;
        hdr.vxlan.vni = meta.tunnel_metadata.vnid;
        hdr.vxlan.reserved2 = 8w0;
    }
    @name(".f_insert_ipv4_header") action f_insert_ipv4_header(bit<8> proto) {
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = proto;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
    }
    @name(".ipv4_vxlan_rewrite") action ipv4_vxlan_rewrite() {
        f_insert_vxlan_header();
        f_insert_ipv4_header(8w17);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w50;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".f_insert_genv_header") action f_insert_genv_header() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.udp.setValid();
        hdr.genv.setValid();
        hdr.udp.srcPort = meta.hash_metadata.entropy_hash;
        hdr.udp.dstPort = 16w6081;
        hdr.udp.checksum = 16w0;
        hdr.udp.length_ = meta.egress_metadata.payload_length + 16w30;
        hdr.genv.ver = 2w0;
        hdr.genv.oam = 1w0;
        hdr.genv.critical = 1w0;
        hdr.genv.optLen = 6w0;
        hdr.genv.protoType = 16w0x6558;
        hdr.genv.vni = meta.tunnel_metadata.vnid;
        hdr.genv.reserved = 6w0;
        hdr.genv.reserved2 = 8w0;
    }
    @name(".ipv4_genv_rewrite") action ipv4_genv_rewrite() {
        f_insert_genv_header();
        f_insert_ipv4_header(8w17);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w50;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".f_insert_nvgre_header") action f_insert_nvgre_header() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.gre.setValid();
        hdr.nvgre.setValid();
        hdr.gre.proto = 16w0x6558;
        hdr.gre.recurse = 3w0;
        hdr.gre.flags = 5w0;
        hdr.gre.ver = 3w0;
        hdr.gre.R = 1w0;
        hdr.gre.K = 1w1;
        hdr.gre.C = 1w0;
        hdr.gre.S = 1w0;
        hdr.gre.s = 1w0;
        hdr.nvgre.tni = meta.tunnel_metadata.vnid;
        hdr.nvgre.flow_id[7:0] = ((bit<8>)meta.hash_metadata.entropy_hash)[7:0];
    }
    @name(".ipv4_nvgre_rewrite") action ipv4_nvgre_rewrite() {
        f_insert_nvgre_header();
        f_insert_ipv4_header(8w47);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w42;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".f_insert_gre_header") action f_insert_gre_header() {
        hdr.gre.setValid();
    }
    @name(".ipv4_gre_rewrite") action ipv4_gre_rewrite() {
        f_insert_gre_header();
        hdr.gre.proto = hdr.ethernet.etherType;
        f_insert_ipv4_header(8w47);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w24;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv4_ip_rewrite") action ipv4_ip_rewrite() {
        f_insert_ipv4_header(meta.tunnel_metadata.inner_ip_proto);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w20;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".f_insert_erspan_t3_header") action f_insert_erspan_t3_header() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.gre.setValid();
        hdr.erspan_t3_header.setValid();
        hdr.gre.C = 1w0;
        hdr.gre.R = 1w0;
        hdr.gre.K = 1w0;
        hdr.gre.S = 1w0;
        hdr.gre.s = 1w0;
        hdr.gre.recurse = 3w0;
        hdr.gre.flags = 5w0;
        hdr.gre.ver = 3w0;
        hdr.gre.proto = 16w0x22eb;
        hdr.erspan_t3_header.timestamp = meta.i2e_metadata.ingress_tstamp;
        hdr.erspan_t3_header.span_id = (bit<10>)meta.i2e_metadata.mirror_session_id;
        hdr.erspan_t3_header.version = 4w2;
        hdr.erspan_t3_header.sgt_other = 32w0;
    }
    @name(".ipv4_erspan_t3_rewrite") action ipv4_erspan_t3_rewrite() {
        f_insert_erspan_t3_header();
        f_insert_ipv4_header(8w47);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w50;
    }
    @name(".f_insert_ipv6_header") action f_insert_ipv6_header(bit<8> proto) {
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = proto;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
    }
    @name(".ipv6_gre_rewrite") action ipv6_gre_rewrite() {
        f_insert_gre_header();
        hdr.gre.proto = hdr.ethernet.etherType;
        f_insert_ipv6_header(8w47);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w4;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_ip_rewrite") action ipv6_ip_rewrite() {
        f_insert_ipv6_header(meta.tunnel_metadata.inner_ip_proto);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_nvgre_rewrite") action ipv6_nvgre_rewrite() {
        f_insert_nvgre_header();
        f_insert_ipv6_header(8w47);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w22;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_vxlan_rewrite") action ipv6_vxlan_rewrite() {
        f_insert_vxlan_header();
        f_insert_ipv6_header(8w17);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w30;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_genv_rewrite") action ipv6_genv_rewrite() {
        f_insert_genv_header();
        f_insert_ipv6_header(8w17);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w30;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_erspan_t3_rewrite") action ipv6_erspan_t3_rewrite() {
        f_insert_erspan_t3_header();
        f_insert_ipv6_header(8w47);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w26;
    }
    @name(".mpls_ethernet_push1_rewrite") action mpls_ethernet_push1_rewrite() {
        hdr.inner_ethernet = hdr.ethernet;
        {
            hdr.mpls.push_front(1);
            hdr.mpls[0].setValid();
        }
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push1_rewrite") action mpls_ip_push1_rewrite() {
        {
            hdr.mpls.push_front(1);
            hdr.mpls[0].setValid();
        }
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ethernet_push2_rewrite") action mpls_ethernet_push2_rewrite() {
        hdr.inner_ethernet = hdr.ethernet;
        {
            hdr.mpls.push_front(2);
            hdr.mpls[0].setValid();
            hdr.mpls[1].setValid();
        }
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push2_rewrite") action mpls_ip_push2_rewrite() {
        {
            hdr.mpls.push_front(2);
            hdr.mpls[0].setValid();
            hdr.mpls[1].setValid();
        }
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ethernet_push3_rewrite") action mpls_ethernet_push3_rewrite() {
        hdr.inner_ethernet = hdr.ethernet;
        {
            hdr.mpls.push_front(3);
            hdr.mpls[0].setValid();
            hdr.mpls[1].setValid();
            hdr.mpls[2].setValid();
        }
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push3_rewrite") action mpls_ip_push3_rewrite() {
        {
            hdr.mpls.push_front(3);
            hdr.mpls[0].setValid();
            hdr.mpls[1].setValid();
            hdr.mpls[2].setValid();
        }
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".fabric_rewrite") action fabric_rewrite(bit<14> tunnel_index) {
        meta.tunnel_metadata.tunnel_index = tunnel_index;
    }
    @name(".tunnel_mtu_check") action tunnel_mtu_check(bit<16> l3_mtu) {
        meta.l3_metadata.l3_mtu_check = l3_mtu |-| meta.egress_metadata.payload_length;
    }
    @name(".tunnel_mtu_miss") action tunnel_mtu_miss() {
        meta.l3_metadata.l3_mtu_check = 16w0xffff;
    }
    @name(".set_tunnel_rewrite_details") action set_tunnel_rewrite_details(bit<16> outer_bd, bit<9> smac_idx, bit<14> dmac_idx, bit<9> sip_index, bit<14> dip_index) {
        meta.egress_metadata.outer_bd = outer_bd;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx;
        meta.tunnel_metadata.tunnel_src_index = sip_index;
        meta.tunnel_metadata.tunnel_dst_index = dip_index;
    }
    @name(".set_mpls_rewrite_push1") action set_mpls_rewrite_push1(bit<20> label1, bit<3> exp1, bit<8> ttl1, bit<9> smac_idx, bit<14> dmac_idx) {
        hdr.mpls[0].label = label1;
        hdr.mpls[0].exp = exp1;
        hdr.mpls[0].bos = 1w0x1;
        hdr.mpls[0].ttl = ttl1;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx;
    }
    @name(".set_mpls_rewrite_push2") action set_mpls_rewrite_push2(bit<20> label1, bit<3> exp1, bit<8> ttl1, bit<20> label2, bit<3> exp2, bit<8> ttl2, bit<9> smac_idx, bit<14> dmac_idx) {
        hdr.mpls[0].label = label1;
        hdr.mpls[0].exp = exp1;
        hdr.mpls[0].ttl = ttl1;
        hdr.mpls[0].bos = 1w0x0;
        hdr.mpls[1].label = label2;
        hdr.mpls[1].exp = exp2;
        hdr.mpls[1].ttl = ttl2;
        hdr.mpls[1].bos = 1w0x1;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx;
    }
    @name(".set_mpls_rewrite_push3") action set_mpls_rewrite_push3(bit<20> label1, bit<3> exp1, bit<8> ttl1, bit<20> label2, bit<3> exp2, bit<8> ttl2, bit<20> label3, bit<3> exp3, bit<8> ttl3, bit<9> smac_idx, bit<14> dmac_idx) {
        hdr.mpls[0].label = label1;
        hdr.mpls[0].exp = exp1;
        hdr.mpls[0].ttl = ttl1;
        hdr.mpls[0].bos = 1w0x0;
        hdr.mpls[1].label = label2;
        hdr.mpls[1].exp = exp2;
        hdr.mpls[1].ttl = ttl2;
        hdr.mpls[1].bos = 1w0x0;
        hdr.mpls[2].label = label3;
        hdr.mpls[2].exp = exp3;
        hdr.mpls[2].ttl = ttl3;
        hdr.mpls[2].bos = 1w0x1;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx;
    }
    @name(".cpu_rx_rewrite") action cpu_rx_rewrite() {
        hdr.fabric_header.setValid();
        hdr.fabric_header.headerVersion = 2w0;
        hdr.fabric_header.packetVersion = 2w0;
        hdr.fabric_header.pad1 = 1w0;
        hdr.fabric_header.packetType = 3w5;
        hdr.fabric_header_cpu.setValid();
        hdr.fabric_header_cpu.ingressPort = (bit<16>)meta.ingress_metadata.ingress_port;
        hdr.fabric_header_cpu.ingressIfindex = meta.ingress_metadata.ifindex;
        hdr.fabric_header_cpu.ingressBd = meta.ingress_metadata.bd;
        hdr.fabric_header_cpu.reasonCode = meta.fabric_metadata.reason_code;
        hdr.fabric_payload_header.setValid();
        hdr.fabric_payload_header.etherType = hdr.ethernet.etherType;
        hdr.ethernet.etherType = 16w0x9000;
    }
    @name(".fabric_unicast_rewrite") action fabric_unicast_rewrite() {
        hdr.fabric_header.setValid();
        hdr.fabric_header.headerVersion = 2w0;
        hdr.fabric_header.packetVersion = 2w0;
        hdr.fabric_header.pad1 = 1w0;
        hdr.fabric_header.packetType = 3w1;
        hdr.fabric_header.dstDevice = meta.fabric_metadata.dst_device;
        hdr.fabric_header.dstPortOrGroup = meta.fabric_metadata.dst_port;
        hdr.fabric_header_unicast.setValid();
        hdr.fabric_header_unicast.tunnelTerminate = meta.tunnel_metadata.tunnel_terminate;
        hdr.fabric_header_unicast.routed = meta.l3_metadata.routed;
        hdr.fabric_header_unicast.outerRouted = meta.l3_metadata.outer_routed;
        hdr.fabric_header_unicast.ingressTunnelType = meta.tunnel_metadata.ingress_tunnel_type;
        hdr.fabric_header_unicast.nexthopIndex = meta.l3_metadata.nexthop_index;
        hdr.fabric_payload_header.setValid();
        hdr.fabric_payload_header.etherType = hdr.ethernet.etherType;
        hdr.ethernet.etherType = 16w0x9000;
    }
    @name(".fabric_multicast_rewrite") action fabric_multicast_rewrite(bit<16> fabric_mgid) {
        hdr.fabric_header.setValid();
        hdr.fabric_header.headerVersion = 2w0;
        hdr.fabric_header.packetVersion = 2w0;
        hdr.fabric_header.pad1 = 1w0;
        hdr.fabric_header.packetType = 3w2;
        hdr.fabric_header.dstDevice = 8w127;
        hdr.fabric_header.dstPortOrGroup = fabric_mgid;
        hdr.fabric_header_multicast.ingressIfindex = meta.ingress_metadata.ifindex;
        hdr.fabric_header_multicast.ingressBd = meta.ingress_metadata.bd;
        hdr.fabric_header_multicast.setValid();
        hdr.fabric_header_multicast.tunnelTerminate = meta.tunnel_metadata.tunnel_terminate;
        hdr.fabric_header_multicast.routed = meta.l3_metadata.routed;
        hdr.fabric_header_multicast.outerRouted = meta.l3_metadata.outer_routed;
        hdr.fabric_header_multicast.ingressTunnelType = meta.tunnel_metadata.ingress_tunnel_type;
        hdr.fabric_header_multicast.mcastGrp = meta.multicast_metadata.mcast_grp;
        hdr.fabric_payload_header.setValid();
        hdr.fabric_payload_header.etherType = hdr.ethernet.etherType;
        hdr.ethernet.etherType = 16w0x9000;
    }
    @name(".rewrite_tunnel_smac") action rewrite_tunnel_smac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name(".rewrite_tunnel_ipv4_src") action rewrite_tunnel_ipv4_src(bit<32> ip) {
        hdr.ipv4.srcAddr = ip;
    }
    @name(".rewrite_tunnel_ipv6_src") action rewrite_tunnel_ipv6_src(bit<128> ip) {
        hdr.ipv6.srcAddr = ip;
    }
    @name(".egress_vni") table egress_vni {
        actions = {
            nop;
            set_egress_tunnel_vni;
        }
        key = {
            meta.egress_metadata.bd                : exact;
            meta.tunnel_metadata.egress_tunnel_type: exact;
        }
        size = 1024;
    }
    @name(".tunnel_dmac_rewrite") table tunnel_dmac_rewrite {
        actions = {
            nop;
            rewrite_tunnel_dmac;
        }
        key = {
            meta.tunnel_metadata.tunnel_dmac_index: exact;
        }
        size = 1024;
    }
    @name(".tunnel_dst_rewrite") table tunnel_dst_rewrite {
        actions = {
            nop;
            rewrite_tunnel_ipv4_dst;
            rewrite_tunnel_ipv6_dst;
        }
        key = {
            meta.tunnel_metadata.tunnel_dst_index: exact;
        }
        size = 1024;
    }
    @name(".tunnel_encap_process_inner") table tunnel_encap_process_inner {
        actions = {
            inner_ipv4_udp_rewrite;
            inner_ipv4_tcp_rewrite;
            inner_ipv4_icmp_rewrite;
            inner_ipv4_unknown_rewrite;
            inner_ipv6_udp_rewrite;
            inner_ipv6_tcp_rewrite;
            inner_ipv6_icmp_rewrite;
            inner_ipv6_unknown_rewrite;
            inner_non_ip_rewrite;
        }
        key = {
            hdr.ipv4.isValid(): exact;
            hdr.ipv6.isValid(): exact;
            hdr.tcp.isValid() : exact;
            hdr.udp.isValid() : exact;
            hdr.icmp.isValid(): exact;
        }
        size = 1024;
    }
    @name(".tunnel_encap_process_outer") table tunnel_encap_process_outer {
        actions = {
            nop;
            ipv4_vxlan_rewrite;
            ipv4_genv_rewrite;
            ipv4_nvgre_rewrite;
            ipv4_gre_rewrite;
            ipv4_ip_rewrite;
            ipv4_erspan_t3_rewrite;
            ipv6_gre_rewrite;
            ipv6_ip_rewrite;
            ipv6_nvgre_rewrite;
            ipv6_vxlan_rewrite;
            ipv6_genv_rewrite;
            ipv6_erspan_t3_rewrite;
            mpls_ethernet_push1_rewrite;
            mpls_ip_push1_rewrite;
            mpls_ethernet_push2_rewrite;
            mpls_ip_push2_rewrite;
            mpls_ethernet_push3_rewrite;
            mpls_ip_push3_rewrite;
            fabric_rewrite;
        }
        key = {
            meta.tunnel_metadata.egress_tunnel_type : exact;
            meta.tunnel_metadata.egress_header_count: exact;
            meta.multicast_metadata.replica         : exact;
        }
        size = 1024;
    }
    @name(".tunnel_mtu") table tunnel_mtu {
        actions = {
            tunnel_mtu_check;
            tunnel_mtu_miss;
        }
        key = {
            meta.tunnel_metadata.tunnel_index: exact;
        }
        size = 1024;
    }
    @name(".tunnel_rewrite") table tunnel_rewrite {
        actions = {
            nop;
            set_tunnel_rewrite_details;
            set_mpls_rewrite_push1;
            set_mpls_rewrite_push2;
            set_mpls_rewrite_push3;
            cpu_rx_rewrite;
            fabric_unicast_rewrite;
            fabric_multicast_rewrite;
        }
        key = {
            meta.tunnel_metadata.tunnel_index: exact;
        }
        size = 1024;
    }
    @name(".tunnel_smac_rewrite") table tunnel_smac_rewrite {
        actions = {
            nop;
            rewrite_tunnel_smac;
        }
        key = {
            meta.tunnel_metadata.tunnel_smac_index: exact;
        }
        size = 1024;
    }
    @name(".tunnel_src_rewrite") table tunnel_src_rewrite {
        actions = {
            nop;
            rewrite_tunnel_ipv4_src;
            rewrite_tunnel_ipv6_src;
        }
        key = {
            meta.tunnel_metadata.tunnel_src_index: exact;
        }
        size = 1024;
    }
    apply {
        if (meta.fabric_metadata.fabric_header_present == 1w0 && meta.tunnel_metadata.egress_tunnel_type != 5w0) {
            egress_vni.apply();
            if (meta.tunnel_metadata.egress_tunnel_type != 5w15 && meta.tunnel_metadata.egress_tunnel_type != 5w16) {
                tunnel_encap_process_inner.apply();
            }
            tunnel_encap_process_outer.apply();
            tunnel_rewrite.apply();
            tunnel_mtu.apply();
            tunnel_src_rewrite.apply();
            tunnel_dst_rewrite.apply();
            tunnel_smac_rewrite.apply();
            tunnel_dmac_rewrite.apply();
        }
    }
}

control process_int_outer_encap(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".int_update_vxlan_gpe_ipv4") action int_update_vxlan_gpe_ipv4() {
        hdr.ipv4.totalLen = hdr.ipv4.totalLen + meta.int_metadata.insert_byte_cnt;
        hdr.udp.length_ = hdr.udp.length_ + meta.int_metadata.insert_byte_cnt;
        hdr.vxlan_gpe_int_header.len = hdr.vxlan_gpe_int_header.len + meta.int_metadata.gpe_int_hdr_len8;
    }
    @name(".int_add_update_vxlan_gpe_ipv4") action int_add_update_vxlan_gpe_ipv4() {
        hdr.vxlan_gpe_int_header.setValid();
        hdr.vxlan_gpe_int_header.int_type = 8w0x1;
        hdr.vxlan_gpe_int_header.next_proto = 8w3;
        hdr.vxlan_gpe.next_proto = 8w5;
        hdr.vxlan_gpe_int_header.len = meta.int_metadata.gpe_int_hdr_len8;
        hdr.ipv4.totalLen = hdr.ipv4.totalLen + meta.int_metadata.insert_byte_cnt;
        hdr.udp.length_ = hdr.udp.length_ + meta.int_metadata.insert_byte_cnt;
    }
    @name(".nop") action nop() {
    }
    @name(".int_outer_encap") table int_outer_encap {
        actions = {
            int_update_vxlan_gpe_ipv4;
            int_add_update_vxlan_gpe_ipv4;
            nop;
        }
        key = {
            hdr.ipv4.isValid()                     : exact;
            hdr.vxlan_gpe.isValid()                : exact;
            meta.int_metadata_i2e.source           : exact;
            meta.tunnel_metadata.egress_tunnel_type: ternary;
        }
        size = 8;
    }
    apply {
        if (meta.int_metadata.insert_cnt != 8w0) {
            int_outer_encap.apply();
        }
    }
}

control process_vlan_xlate(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_egress_packet_vlan_untagged") action set_egress_packet_vlan_untagged() {
    }
    @name(".set_egress_packet_vlan_tagged") action set_egress_packet_vlan_tagged(bit<12> vlan_id) {
        hdr.vlan_tag_[0].setValid();
        hdr.vlan_tag_[0].etherType = hdr.ethernet.etherType;
        hdr.vlan_tag_[0].vid = vlan_id;
        hdr.ethernet.etherType = 16w0x8100;
    }
    @name(".set_egress_packet_vlan_double_tagged") action set_egress_packet_vlan_double_tagged(bit<12> s_tag, bit<12> c_tag) {
        hdr.vlan_tag_[1].setValid();
        hdr.vlan_tag_[0].setValid();
        hdr.vlan_tag_[1].etherType = hdr.ethernet.etherType;
        hdr.vlan_tag_[1].vid = c_tag;
        hdr.vlan_tag_[0].etherType = 16w0x8100;
        hdr.vlan_tag_[0].vid = s_tag;
        hdr.ethernet.etherType = 16w0x9100;
    }
    @name(".egress_vlan_xlate") table egress_vlan_xlate {
        actions = {
            set_egress_packet_vlan_untagged;
            set_egress_packet_vlan_tagged;
            set_egress_packet_vlan_double_tagged;
        }
        key = {
            meta.egress_metadata.ifindex: exact;
            meta.egress_metadata.bd     : exact;
        }
        size = 1024;
    }
    apply {
        egress_vlan_xlate.apply();
    }
}

control process_egress_filter(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".egress_filter_check") action egress_filter_check() {
        meta.egress_filter_metadata.ifindex_check = meta.ingress_metadata.ifindex ^ meta.egress_metadata.ifindex;
        meta.egress_filter_metadata.bd = meta.ingress_metadata.outer_bd ^ meta.egress_metadata.outer_bd;
        meta.egress_filter_metadata.inner_bd = meta.ingress_metadata.bd ^ meta.egress_metadata.bd;
    }
    @name(".set_egress_filter_drop") action set_egress_filter_drop() {
        mark_to_drop(standard_metadata);
    }
    @name(".egress_filter") table egress_filter {
        actions = {
            egress_filter_check;
        }
    }
    @name(".egress_filter_drop") table egress_filter_drop {
        actions = {
            set_egress_filter_drop;
        }
    }
    apply {
        egress_filter.apply();
        if (meta.multicast_metadata.inner_replica == 1w1) {
            if (meta.tunnel_metadata.ingress_tunnel_type == 5w0 && meta.tunnel_metadata.egress_tunnel_type == 5w0 && meta.egress_filter_metadata.bd == 16w0 && meta.egress_filter_metadata.ifindex_check == 16w0 || meta.tunnel_metadata.ingress_tunnel_type != 5w0 && meta.tunnel_metadata.egress_tunnel_type != 5w0 && meta.egress_filter_metadata.inner_bd == 16w0) {
                egress_filter_drop.apply();
            }
        }
    }
}

control process_egress_acl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".egress_mirror") action egress_mirror(bit<32> session_id) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        clone3(CloneType.E2E, (bit<32>)session_id, { meta.i2e_metadata.ingress_tstamp, meta.i2e_metadata.mirror_session_id });
    }
    @name(".egress_mirror_drop") action egress_mirror_drop(bit<32> session_id) {
        egress_mirror(session_id);
        mark_to_drop(standard_metadata);
    }
    @name(".egress_copy_to_cpu") action egress_copy_to_cpu(bit<16> reason_code) {
        meta.fabric_metadata.reason_code = reason_code;
        clone3(CloneType.E2E, (bit<32>)32w250, { meta.ingress_metadata.bd, meta.ingress_metadata.ifindex, meta.fabric_metadata.reason_code, meta.ingress_metadata.ingress_port });
    }
    @name(".egress_redirect_to_cpu") action egress_redirect_to_cpu(bit<16> reason_code) {
        egress_copy_to_cpu(reason_code);
        mark_to_drop(standard_metadata);
    }
    @name(".egress_acl") table egress_acl {
        actions = {
            nop;
            egress_mirror;
            egress_mirror_drop;
            egress_redirect_to_cpu;
        }
        key = {
            standard_metadata.egress_port: ternary;
            meta.l3_metadata.l3_mtu_check: ternary;
        }
        size = 512;
    }
    apply {
        if (meta.egress_metadata.bypass == 1w0) {
            egress_acl.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".egress_port_type_normal") action egress_port_type_normal(bit<16> ifindex) {
        meta.egress_metadata.port_type = 2w0;
        meta.egress_metadata.ifindex = ifindex;
    }
    @name(".egress_port_type_fabric") action egress_port_type_fabric(bit<16> ifindex) {
        meta.egress_metadata.port_type = 2w1;
        meta.egress_metadata.ifindex = ifindex;
        meta.tunnel_metadata.egress_tunnel_type = 5w15;
        meta.egress_metadata.ifindex = ifindex;
    }
    @name(".egress_port_type_cpu") action egress_port_type_cpu(bit<16> ifindex) {
        meta.egress_metadata.port_type = 2w2;
        meta.egress_metadata.ifindex = ifindex;
        meta.tunnel_metadata.egress_tunnel_type = 5w16;
        meta.egress_metadata.ifindex = ifindex;
    }
    @name(".nop") action nop() {
    }
    @name(".set_mirror_nhop") action set_mirror_nhop(bit<16> nhop_idx) {
        meta.l3_metadata.nexthop_index = nhop_idx;
    }
    @name(".set_mirror_bd") action set_mirror_bd(bit<16> bd) {
        meta.egress_metadata.bd = bd;
    }
    @name(".sflow_pkt_to_cpu") action sflow_pkt_to_cpu() {
        hdr.fabric_header_sflow.setValid();
        hdr.fabric_header_sflow.sflow_session_id = meta.sflow_metadata.sflow_session_id;
    }
    @name(".egress_port_mapping") table egress_port_mapping {
        actions = {
            egress_port_type_normal;
            egress_port_type_fabric;
            egress_port_type_cpu;
        }
        key = {
            standard_metadata.egress_port: exact;
        }
        size = 288;
    }
    @name(".mirror") table mirror {
        actions = {
            nop;
            set_mirror_nhop;
            set_mirror_bd;
            sflow_pkt_to_cpu;
        }
        key = {
            meta.i2e_metadata.mirror_session_id: exact;
        }
        size = 1024;
    }
    @name(".process_replication") process_replication() process_replication_0;
    @name(".process_vlan_decap") process_vlan_decap() process_vlan_decap_0;
    @name(".process_tunnel_decap") process_tunnel_decap() process_tunnel_decap_0;
    @name(".process_rewrite") process_rewrite() process_rewrite_0;
    @name(".process_egress_bd") process_egress_bd() process_egress_bd_0;
    @name(".process_mac_rewrite") process_mac_rewrite() process_mac_rewrite_0;
    @name(".process_mtu") process_mtu() process_mtu_0;
    @name(".process_int_insertion") process_int_insertion() process_int_insertion_0;
    @name(".process_egress_bd_stats") process_egress_bd_stats() process_egress_bd_stats_0;
    @name(".process_tunnel_encap") process_tunnel_encap() process_tunnel_encap_0;
    @name(".process_int_outer_encap") process_int_outer_encap() process_int_outer_encap_0;
    @name(".process_vlan_xlate") process_vlan_xlate() process_vlan_xlate_0;
    @name(".process_egress_filter") process_egress_filter() process_egress_filter_0;
    @name(".process_egress_acl") process_egress_acl() process_egress_acl_0;
    apply {
        if (meta.egress_metadata.bypass == 1w0) {
            if (standard_metadata.instance_type != 32w0 && standard_metadata.instance_type != 32w5) {
                mirror.apply();
            } else {
                process_replication_0.apply(hdr, meta, standard_metadata);
            }
            switch (egress_port_mapping.apply().action_run) {
                egress_port_type_normal: {
                    if (standard_metadata.instance_type == 32w0 || standard_metadata.instance_type == 32w5) {
                        process_vlan_decap_0.apply(hdr, meta, standard_metadata);
                    }
                    process_tunnel_decap_0.apply(hdr, meta, standard_metadata);
                    process_rewrite_0.apply(hdr, meta, standard_metadata);
                    process_egress_bd_0.apply(hdr, meta, standard_metadata);
                    process_mac_rewrite_0.apply(hdr, meta, standard_metadata);
                    process_mtu_0.apply(hdr, meta, standard_metadata);
                    process_int_insertion_0.apply(hdr, meta, standard_metadata);
                    process_egress_bd_stats_0.apply(hdr, meta, standard_metadata);
                }
            }

            process_tunnel_encap_0.apply(hdr, meta, standard_metadata);
            process_int_outer_encap_0.apply(hdr, meta, standard_metadata);
            if (meta.egress_metadata.port_type == 2w0) {
                process_vlan_xlate_0.apply(hdr, meta, standard_metadata);
            }
            process_egress_filter_0.apply(hdr, meta, standard_metadata);
        }
        process_egress_acl_0.apply(hdr, meta, standard_metadata);
    }
}

control process_ingress_port_mapping(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_ifindex") action set_ifindex(bit<16> ifindex, bit<2> port_type) {
        meta.ingress_metadata.ifindex = ifindex;
        meta.ingress_metadata.port_type = port_type;
    }
    @name(".set_ingress_port_properties") action set_ingress_port_properties(bit<16> if_label) {
        meta.acl_metadata.if_label = if_label;
    }
    @name(".ingress_port_mapping") table ingress_port_mapping {
        actions = {
            set_ifindex;
        }
        key = {
            standard_metadata.ingress_port: exact;
        }
        size = 288;
    }
    @name(".ingress_port_properties") table ingress_port_properties {
        actions = {
            set_ingress_port_properties;
        }
        key = {
            standard_metadata.ingress_port: exact;
        }
        size = 288;
    }
    apply {
        ingress_port_mapping.apply();
        ingress_port_properties.apply();
    }
}

control validate_outer_ipv4_header(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_valid_outer_ipv4_packet") action set_valid_outer_ipv4_packet() {
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.l3_metadata.lkp_ip_tc = hdr.ipv4.diffserv;
        meta.l3_metadata.lkp_ip_version = hdr.ipv4.version;
    }
    @name(".set_malformed_outer_ipv4_packet") action set_malformed_outer_ipv4_packet(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
    }
    @name(".validate_outer_ipv4_packet") table validate_outer_ipv4_packet {
        actions = {
            set_valid_outer_ipv4_packet;
            set_malformed_outer_ipv4_packet;
        }
        key = {
            hdr.ipv4.version       : ternary;
            hdr.ipv4.ttl           : ternary;
            hdr.ipv4.srcAddr[31:24]: ternary @name("ipv4.srcAddr") ;
        }
        size = 512;
    }
    apply {
        validate_outer_ipv4_packet.apply();
    }
}

control validate_outer_ipv6_header(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_valid_outer_ipv6_packet") action set_valid_outer_ipv6_packet() {
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.l3_metadata.lkp_ip_tc = hdr.ipv6.trafficClass;
        meta.l3_metadata.lkp_ip_version = hdr.ipv6.version;
    }
    @name(".set_malformed_outer_ipv6_packet") action set_malformed_outer_ipv6_packet(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
    }
    @name(".validate_outer_ipv6_packet") table validate_outer_ipv6_packet {
        actions = {
            set_valid_outer_ipv6_packet;
            set_malformed_outer_ipv6_packet;
        }
        key = {
            hdr.ipv6.version         : ternary;
            hdr.ipv6.hopLimit        : ternary;
            hdr.ipv6.srcAddr[127:112]: ternary @name("ipv6.srcAddr") ;
        }
        size = 512;
    }
    apply {
        validate_outer_ipv6_packet.apply();
    }
}

control validate_mpls_header(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_valid_mpls_label1") action set_valid_mpls_label1() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[0].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[0].exp;
    }
    @name(".set_valid_mpls_label2") action set_valid_mpls_label2() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[1].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[1].exp;
    }
    @name(".set_valid_mpls_label3") action set_valid_mpls_label3() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[2].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[2].exp;
    }
    @name(".validate_mpls_packet") table validate_mpls_packet {
        actions = {
            set_valid_mpls_label1;
            set_valid_mpls_label2;
            set_valid_mpls_label3;
        }
        key = {
            hdr.mpls[0].label    : ternary;
            hdr.mpls[0].bos      : ternary;
            hdr.mpls[0].isValid(): exact;
            hdr.mpls[1].label    : ternary;
            hdr.mpls[1].bos      : ternary;
            hdr.mpls[1].isValid(): exact;
            hdr.mpls[2].label    : ternary;
            hdr.mpls[2].bos      : ternary;
            hdr.mpls[2].isValid(): exact;
        }
        size = 512;
    }
    apply {
        validate_mpls_packet.apply();
    }
}

control process_validate_outer_header(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".malformed_outer_ethernet_packet") action malformed_outer_ethernet_packet(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
    }
    @name(".set_valid_outer_unicast_packet_untagged") action set_valid_outer_unicast_packet_untagged() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_unicast_packet_single_tagged") action set_valid_outer_unicast_packet_single_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
    }
    @name(".set_valid_outer_unicast_packet_double_tagged") action set_valid_outer_unicast_packet_double_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
    }
    @name(".set_valid_outer_unicast_packet_qinq_tagged") action set_valid_outer_unicast_packet_qinq_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_multicast_packet_untagged") action set_valid_outer_multicast_packet_untagged() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_multicast_packet_single_tagged") action set_valid_outer_multicast_packet_single_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
    }
    @name(".set_valid_outer_multicast_packet_double_tagged") action set_valid_outer_multicast_packet_double_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
    }
    @name(".set_valid_outer_multicast_packet_qinq_tagged") action set_valid_outer_multicast_packet_qinq_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_broadcast_packet_untagged") action set_valid_outer_broadcast_packet_untagged() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_broadcast_packet_single_tagged") action set_valid_outer_broadcast_packet_single_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
    }
    @name(".set_valid_outer_broadcast_packet_double_tagged") action set_valid_outer_broadcast_packet_double_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
    }
    @name(".set_valid_outer_broadcast_packet_qinq_tagged") action set_valid_outer_broadcast_packet_qinq_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".validate_outer_ethernet") table validate_outer_ethernet {
        actions = {
            malformed_outer_ethernet_packet;
            set_valid_outer_unicast_packet_untagged;
            set_valid_outer_unicast_packet_single_tagged;
            set_valid_outer_unicast_packet_double_tagged;
            set_valid_outer_unicast_packet_qinq_tagged;
            set_valid_outer_multicast_packet_untagged;
            set_valid_outer_multicast_packet_single_tagged;
            set_valid_outer_multicast_packet_double_tagged;
            set_valid_outer_multicast_packet_qinq_tagged;
            set_valid_outer_broadcast_packet_untagged;
            set_valid_outer_broadcast_packet_single_tagged;
            set_valid_outer_broadcast_packet_double_tagged;
            set_valid_outer_broadcast_packet_qinq_tagged;
        }
        key = {
            hdr.ethernet.srcAddr      : ternary;
            hdr.ethernet.dstAddr      : ternary;
            hdr.vlan_tag_[0].isValid(): exact;
            hdr.vlan_tag_[1].isValid(): exact;
        }
        size = 512;
    }
    @name(".validate_outer_ipv4_header") validate_outer_ipv4_header() validate_outer_ipv4_header_0;
    @name(".validate_outer_ipv6_header") validate_outer_ipv6_header() validate_outer_ipv6_header_0;
    @name(".validate_mpls_header") validate_mpls_header() validate_mpls_header_0;
    apply {
        switch (validate_outer_ethernet.apply().action_run) {
            malformed_outer_ethernet_packet: {
            }
            default: {
                if (hdr.ipv4.isValid()) {
                    validate_outer_ipv4_header_0.apply(hdr, meta, standard_metadata);
                } else {
                    if (hdr.ipv6.isValid()) {
                        validate_outer_ipv6_header_0.apply(hdr, meta, standard_metadata);
                    } else {
                        if (hdr.mpls[0].isValid()) {
                            validate_mpls_header_0.apply(hdr, meta, standard_metadata);
                        }
                    }
                }
            }
        }

    }
}

control process_global_params(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".deflect_on_drop") action deflect_on_drop(bit<8> enable_dod) {
    }
    @name(".set_config_parameters") action set_config_parameters(bit<8> enable_dod) {
        deflect_on_drop(enable_dod);
        meta.i2e_metadata.ingress_tstamp = (bit<32>)standard_metadata.ingress_global_timestamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
        standard_metadata.egress_spec = 9w511;
        random(meta.ingress_metadata.sflow_take_sample, (bit<32>)0, 32w0x7fffffff);
    }
    @name(".switch_config_params") table switch_config_params {
        actions = {
            set_config_parameters;
        }
        size = 1;
    }
    apply {
        switch_config_params.apply();
    }
}

control process_port_vlan_mapping(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_bd_properties") action set_bd_properties(bit<16> bd, bit<16> vrf, bit<10> stp_group, bit<1> learning_enabled, bit<16> bd_label, bit<16> stats_idx, bit<10> rmac_group, bit<1> ipv4_unicast_enabled, bit<1> ipv6_unicast_enabled, bit<2> ipv4_urpf_mode, bit<2> ipv6_urpf_mode, bit<1> igmp_snooping_enabled, bit<1> mld_snooping_enabled, bit<1> ipv4_multicast_enabled, bit<1> ipv6_multicast_enabled, bit<16> mrpf_group, bit<16> ipv4_mcast_key, bit<1> ipv4_mcast_key_type, bit<16> ipv6_mcast_key, bit<1> ipv6_mcast_key_type) {
        meta.ingress_metadata.bd = bd;
        meta.ingress_metadata.outer_bd = bd;
        meta.acl_metadata.bd_label = bd_label;
        meta.l2_metadata.stp_group = stp_group;
        meta.l2_metadata.bd_stats_idx = stats_idx;
        meta.l2_metadata.learning_enabled = learning_enabled;
        meta.l3_metadata.vrf = vrf;
        meta.ipv4_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled;
        meta.ipv6_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled;
        meta.ipv4_metadata.ipv4_urpf_mode = ipv4_urpf_mode;
        meta.ipv6_metadata.ipv6_urpf_mode = ipv6_urpf_mode;
        meta.l3_metadata.rmac_group = rmac_group;
        meta.multicast_metadata.igmp_snooping_enabled = igmp_snooping_enabled;
        meta.multicast_metadata.mld_snooping_enabled = mld_snooping_enabled;
        meta.multicast_metadata.ipv4_multicast_enabled = ipv4_multicast_enabled;
        meta.multicast_metadata.ipv6_multicast_enabled = ipv6_multicast_enabled;
        meta.multicast_metadata.bd_mrpf_group = mrpf_group;
        meta.multicast_metadata.ipv4_mcast_key_type = ipv4_mcast_key_type;
        meta.multicast_metadata.ipv4_mcast_key = ipv4_mcast_key;
        meta.multicast_metadata.ipv6_mcast_key_type = ipv6_mcast_key_type;
        meta.multicast_metadata.ipv6_mcast_key = ipv6_mcast_key;
    }
    @name(".port_vlan_mapping_miss") action port_vlan_mapping_miss() {
        meta.l2_metadata.port_vlan_mapping_miss = 1w1;
    }
    @name(".port_vlan_mapping") table port_vlan_mapping {
        actions = {
            set_bd_properties;
            port_vlan_mapping_miss;
        }
        key = {
            meta.ingress_metadata.ifindex: exact;
            hdr.vlan_tag_[0].isValid()   : exact;
            hdr.vlan_tag_[0].vid         : exact;
            hdr.vlan_tag_[1].isValid()   : exact;
            hdr.vlan_tag_[1].vid         : exact;
        }
        size = 4096;
        implementation = bd_action_profile;
    }
    apply {
        port_vlan_mapping.apply();
    }
}

control process_spanning_tree(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_stp_state") action set_stp_state(bit<3> stp_state) {
        meta.l2_metadata.stp_state = stp_state;
    }
    @name(".spanning_tree") table spanning_tree {
        actions = {
            set_stp_state;
        }
        key = {
            meta.ingress_metadata.ifindex: exact;
            meta.l2_metadata.stp_group   : exact;
        }
        size = 1024;
    }
    apply {
        if (meta.ingress_metadata.port_type == 2w0 && meta.l2_metadata.stp_group != 10w0) {
            spanning_tree.apply();
        }
    }
}

control process_ip_sourceguard(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss() {
    }
    @name(".ipsg_miss") action ipsg_miss() {
        meta.security_metadata.ipsg_check_fail = 1w1;
    }
    @name(".ipsg") table ipsg {
        actions = {
            on_miss;
        }
        key = {
            meta.ingress_metadata.ifindex : exact;
            meta.ingress_metadata.bd      : exact;
            meta.l2_metadata.lkp_mac_sa   : exact;
            meta.ipv4_metadata.lkp_ipv4_sa: exact;
        }
        size = 1024;
    }
    @name(".ipsg_permit_special") table ipsg_permit_special {
        actions = {
            ipsg_miss;
        }
        key = {
            meta.l3_metadata.lkp_ip_proto : ternary;
            meta.l3_metadata.lkp_l4_dport : ternary;
            meta.ipv4_metadata.lkp_ipv4_da: ternary;
        }
        size = 512;
    }
    apply {
        if (meta.ingress_metadata.port_type == 2w0 && meta.security_metadata.ipsg_enabled == 1w1) {
            switch (ipsg.apply().action_run) {
                on_miss: {
                    ipsg_permit_special.apply();
                }
            }

        }
    }
}

control process_int_endpoint(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".int_sink_update_vxlan_gpe_v4") action int_sink_update_vxlan_gpe_v4() {
        hdr.vxlan_gpe.next_proto = hdr.vxlan_gpe_int_header.next_proto;
        hdr.vxlan_gpe_int_header.setInvalid();
        hdr.ipv4.totalLen = hdr.ipv4.totalLen - meta.int_metadata.insert_byte_cnt;
        hdr.udp.length_ = hdr.udp.length_ - meta.int_metadata.insert_byte_cnt;
    }
    @name(".nop") action nop() {
    }
    @name(".int_set_src") action int_set_src() {
        meta.int_metadata_i2e.source = 1w1;
    }
    @name(".int_set_no_src") action int_set_no_src() {
        meta.int_metadata_i2e.source = 1w0;
    }
    @name(".int_sink") action int_sink(bit<32> mirror_id) {
        meta.int_metadata_i2e.sink = 1w1;
        meta.i2e_metadata.mirror_session_id = (bit<16>)mirror_id;
        clone3(CloneType.I2E, (bit<32>)mirror_id, { meta.int_metadata_i2e.sink, meta.i2e_metadata.mirror_session_id });
        hdr.int_header.setInvalid();
        hdr.int_val[0].setInvalid();
        hdr.int_val[1].setInvalid();
        hdr.int_val[2].setInvalid();
        hdr.int_val[3].setInvalid();
        hdr.int_val[4].setInvalid();
        hdr.int_val[5].setInvalid();
        hdr.int_val[6].setInvalid();
        hdr.int_val[7].setInvalid();
        hdr.int_val[8].setInvalid();
        hdr.int_val[9].setInvalid();
        hdr.int_val[10].setInvalid();
        hdr.int_val[11].setInvalid();
        hdr.int_val[12].setInvalid();
        hdr.int_val[13].setInvalid();
        hdr.int_val[14].setInvalid();
        hdr.int_val[15].setInvalid();
        hdr.int_val[16].setInvalid();
        hdr.int_val[17].setInvalid();
        hdr.int_val[18].setInvalid();
        hdr.int_val[19].setInvalid();
        hdr.int_val[20].setInvalid();
        hdr.int_val[21].setInvalid();
        hdr.int_val[22].setInvalid();
        hdr.int_val[23].setInvalid();
    }
    @name(".int_sink_gpe") action int_sink_gpe(bit<32> mirror_id) {
        meta.int_metadata.insert_byte_cnt = meta.int_metadata.gpe_int_hdr_len << 2;
        int_sink(mirror_id);
    }
    @name(".int_no_sink") action int_no_sink() {
        meta.int_metadata_i2e.sink = 1w0;
    }
    @name(".int_sink_update_outer") table int_sink_update_outer {
        actions = {
            int_sink_update_vxlan_gpe_v4;
            nop;
        }
        key = {
            hdr.vxlan_gpe_int_header.isValid(): exact;
            hdr.ipv4.isValid()                : exact;
            meta.int_metadata_i2e.sink        : exact;
        }
        size = 2;
    }
    @name(".int_source") table int_source {
        actions = {
            int_set_src;
            int_set_no_src;
        }
        key = {
            hdr.int_header.isValid()      : exact;
            hdr.ipv4.isValid()            : exact;
            meta.ipv4_metadata.lkp_ipv4_da: ternary;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary;
            hdr.inner_ipv4.isValid()      : exact;
            hdr.inner_ipv4.dstAddr        : ternary;
            hdr.inner_ipv4.srcAddr        : ternary;
        }
        size = 256;
    }
    @name(".int_terminate") table int_terminate {
        actions = {
            int_sink_gpe;
            int_no_sink;
        }
        key = {
            hdr.int_header.isValid()          : exact;
            hdr.vxlan_gpe_int_header.isValid(): exact;
            hdr.ipv4.isValid()                : exact;
            meta.ipv4_metadata.lkp_ipv4_da    : ternary;
            hdr.inner_ipv4.isValid()          : exact;
            hdr.inner_ipv4.dstAddr            : ternary;
        }
        size = 256;
    }
    apply {
        if (!hdr.int_header.isValid()) {
            int_source.apply();
        } else {
            int_terminate.apply();
            int_sink_update_outer.apply();
        }
    }
}

control process_ingress_fabric(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".terminate_cpu_packet") action terminate_cpu_packet() {
        standard_metadata.egress_spec = (bit<9>)hdr.fabric_header.dstPortOrGroup;
        meta.egress_metadata.bypass = hdr.fabric_header_cpu.txBypass;
        hdr.ethernet.etherType = hdr.fabric_payload_header.etherType;
        hdr.fabric_header.setInvalid();
        hdr.fabric_header_cpu.setInvalid();
        hdr.fabric_payload_header.setInvalid();
    }
    @name(".switch_fabric_unicast_packet") action switch_fabric_unicast_packet() {
        meta.fabric_metadata.fabric_header_present = 1w1;
        meta.fabric_metadata.dst_device = hdr.fabric_header.dstDevice;
        meta.fabric_metadata.dst_port = hdr.fabric_header.dstPortOrGroup;
    }
    @name(".terminate_fabric_unicast_packet") action terminate_fabric_unicast_packet() {
        standard_metadata.egress_spec = (bit<9>)hdr.fabric_header.dstPortOrGroup;
        meta.tunnel_metadata.tunnel_terminate = hdr.fabric_header_unicast.tunnelTerminate;
        meta.tunnel_metadata.ingress_tunnel_type = hdr.fabric_header_unicast.ingressTunnelType;
        meta.l3_metadata.nexthop_index = hdr.fabric_header_unicast.nexthopIndex;
        meta.l3_metadata.routed = hdr.fabric_header_unicast.routed;
        meta.l3_metadata.outer_routed = hdr.fabric_header_unicast.outerRouted;
        hdr.ethernet.etherType = hdr.fabric_payload_header.etherType;
        hdr.fabric_header.setInvalid();
        hdr.fabric_header_unicast.setInvalid();
        hdr.fabric_payload_header.setInvalid();
    }
    @name(".switch_fabric_multicast_packet") action switch_fabric_multicast_packet() {
        meta.fabric_metadata.fabric_header_present = 1w1;
        standard_metadata.mcast_grp = hdr.fabric_header.dstPortOrGroup;
    }
    @name(".terminate_fabric_multicast_packet") action terminate_fabric_multicast_packet() {
        meta.tunnel_metadata.tunnel_terminate = hdr.fabric_header_multicast.tunnelTerminate;
        meta.tunnel_metadata.ingress_tunnel_type = hdr.fabric_header_multicast.ingressTunnelType;
        meta.l3_metadata.nexthop_index = 16w0;
        meta.l3_metadata.routed = hdr.fabric_header_multicast.routed;
        meta.l3_metadata.outer_routed = hdr.fabric_header_multicast.outerRouted;
        standard_metadata.mcast_grp = hdr.fabric_header_multicast.mcastGrp;
        hdr.ethernet.etherType = hdr.fabric_payload_header.etherType;
        hdr.fabric_header.setInvalid();
        hdr.fabric_header_multicast.setInvalid();
        hdr.fabric_payload_header.setInvalid();
    }
    @name(".set_ingress_ifindex_properties") action set_ingress_ifindex_properties() {
    }
    @name(".non_ip_over_fabric") action non_ip_over_fabric() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".ipv4_over_fabric") action ipv4_over_fabric() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.ipv4.protocol;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_outer_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_outer_l4_dport;
    }
    @name(".ipv6_over_fabric") action ipv6_over_fabric() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.ipv6.nextHdr;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_outer_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_outer_l4_dport;
    }
    @name(".fabric_ingress_dst_lkp") table fabric_ingress_dst_lkp {
        actions = {
            nop;
            terminate_cpu_packet;
            switch_fabric_unicast_packet;
            terminate_fabric_unicast_packet;
            switch_fabric_multicast_packet;
            terminate_fabric_multicast_packet;
        }
        key = {
            hdr.fabric_header.dstDevice: exact;
        }
    }
    @name(".fabric_ingress_src_lkp") table fabric_ingress_src_lkp {
        actions = {
            nop;
            set_ingress_ifindex_properties;
        }
        key = {
            hdr.fabric_header_multicast.ingressIfindex: exact;
        }
        size = 1024;
    }
    @name(".native_packet_over_fabric") table native_packet_over_fabric {
        actions = {
            non_ip_over_fabric;
            ipv4_over_fabric;
            ipv6_over_fabric;
        }
        key = {
            hdr.ipv4.isValid(): exact;
            hdr.ipv6.isValid(): exact;
        }
        size = 1024;
    }
    apply {
        if (meta.ingress_metadata.port_type != 2w0) {
            fabric_ingress_dst_lkp.apply();
            if (meta.ingress_metadata.port_type == 2w1) {
                if (hdr.fabric_header_multicast.isValid()) {
                    fabric_ingress_src_lkp.apply();
                }
                if (meta.tunnel_metadata.tunnel_terminate == 1w0) {
                    native_packet_over_fabric.apply();
                }
            }
        }
    }
}

control process_outer_ipv4_multicast(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".on_miss") action on_miss() {
    }
    @name(".outer_multicast_route_s_g_hit") action outer_multicast_route_s_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        standard_metadata.mcast_grp = mc_index;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_bridge_s_g_hit") action outer_multicast_bridge_s_g_hit(bit<16> mc_index) {
        standard_metadata.mcast_grp = mc_index;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_route_sm_star_g_hit") action outer_multicast_route_sm_star_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        meta.multicast_metadata.outer_mcast_mode = 2w1;
        standard_metadata.mcast_grp = mc_index;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_route_bidir_star_g_hit") action outer_multicast_route_bidir_star_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        meta.multicast_metadata.outer_mcast_mode = 2w2;
        standard_metadata.mcast_grp = mc_index;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group | meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_bridge_star_g_hit") action outer_multicast_bridge_star_g_hit(bit<16> mc_index) {
        standard_metadata.mcast_grp = mc_index;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_ipv4_multicast") table outer_ipv4_multicast {
        actions = {
            nop;
            on_miss;
            outer_multicast_route_s_g_hit;
            outer_multicast_bridge_s_g_hit;
        }
        key = {
            meta.multicast_metadata.ipv4_mcast_key_type: exact;
            meta.multicast_metadata.ipv4_mcast_key     : exact;
            hdr.ipv4.srcAddr                           : exact;
            hdr.ipv4.dstAddr                           : exact;
        }
        size = 1024;
    }
    @name(".outer_ipv4_multicast_star_g") table outer_ipv4_multicast_star_g {
        actions = {
            nop;
            outer_multicast_route_sm_star_g_hit;
            outer_multicast_route_bidir_star_g_hit;
            outer_multicast_bridge_star_g_hit;
        }
        key = {
            meta.multicast_metadata.ipv4_mcast_key_type: exact;
            meta.multicast_metadata.ipv4_mcast_key     : exact;
            hdr.ipv4.dstAddr                           : ternary;
        }
        size = 512;
    }
    apply {
        switch (outer_ipv4_multicast.apply().action_run) {
            on_miss: {
                outer_ipv4_multicast_star_g.apply();
            }
        }

    }
}

control process_outer_ipv6_multicast(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".on_miss") action on_miss() {
    }
    @name(".outer_multicast_route_s_g_hit") action outer_multicast_route_s_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        standard_metadata.mcast_grp = mc_index;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_bridge_s_g_hit") action outer_multicast_bridge_s_g_hit(bit<16> mc_index) {
        standard_metadata.mcast_grp = mc_index;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_route_sm_star_g_hit") action outer_multicast_route_sm_star_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        meta.multicast_metadata.outer_mcast_mode = 2w1;
        standard_metadata.mcast_grp = mc_index;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_route_bidir_star_g_hit") action outer_multicast_route_bidir_star_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        meta.multicast_metadata.outer_mcast_mode = 2w2;
        standard_metadata.mcast_grp = mc_index;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group | meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_bridge_star_g_hit") action outer_multicast_bridge_star_g_hit(bit<16> mc_index) {
        standard_metadata.mcast_grp = mc_index;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_ipv6_multicast") table outer_ipv6_multicast {
        actions = {
            nop;
            on_miss;
            outer_multicast_route_s_g_hit;
            outer_multicast_bridge_s_g_hit;
        }
        key = {
            meta.multicast_metadata.ipv6_mcast_key_type: exact;
            meta.multicast_metadata.ipv6_mcast_key     : exact;
            hdr.ipv6.srcAddr                           : exact;
            hdr.ipv6.dstAddr                           : exact;
        }
        size = 1024;
    }
    @name(".outer_ipv6_multicast_star_g") table outer_ipv6_multicast_star_g {
        actions = {
            nop;
            outer_multicast_route_sm_star_g_hit;
            outer_multicast_route_bidir_star_g_hit;
            outer_multicast_bridge_star_g_hit;
        }
        key = {
            meta.multicast_metadata.ipv6_mcast_key_type: exact;
            meta.multicast_metadata.ipv6_mcast_key     : exact;
            hdr.ipv6.dstAddr                           : ternary;
        }
        size = 512;
    }
    apply {
        switch (outer_ipv6_multicast.apply().action_run) {
            on_miss: {
                outer_ipv6_multicast_star_g.apply();
            }
        }

    }
}

control process_outer_multicast_rpf(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control process_outer_multicast(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".process_outer_ipv4_multicast") process_outer_ipv4_multicast() process_outer_ipv4_multicast_0;
    @name(".process_outer_ipv6_multicast") process_outer_ipv6_multicast() process_outer_ipv6_multicast_0;
    @name(".process_outer_multicast_rpf") process_outer_multicast_rpf() process_outer_multicast_rpf_0;
    apply {
        if (hdr.ipv4.isValid()) {
            process_outer_ipv4_multicast_0.apply(hdr, meta, standard_metadata);
        } else {
            if (hdr.ipv6.isValid()) {
                process_outer_ipv6_multicast_0.apply(hdr, meta, standard_metadata);
            }
        }
        process_outer_multicast_rpf_0.apply(hdr, meta, standard_metadata);
    }
}

control process_ipv4_vtep(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_tunnel_termination_flag") action set_tunnel_termination_flag() {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".set_tunnel_vni_and_termination_flag") action set_tunnel_vni_and_termination_flag(bit<24> tunnel_vni) {
        meta.tunnel_metadata.tunnel_vni = tunnel_vni;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".on_miss") action on_miss() {
    }
    @name(".src_vtep_hit") action src_vtep_hit(bit<16> ifindex) {
        meta.ingress_metadata.ifindex = ifindex;
    }
    @name(".ipv4_dest_vtep") table ipv4_dest_vtep {
        actions = {
            nop;
            set_tunnel_termination_flag;
            set_tunnel_vni_and_termination_flag;
        }
        key = {
            meta.l3_metadata.vrf                    : exact;
            hdr.ipv4.dstAddr                        : exact;
            meta.tunnel_metadata.ingress_tunnel_type: exact;
        }
        size = 1024;
    }
    @name(".ipv4_src_vtep") table ipv4_src_vtep {
        actions = {
            on_miss;
            src_vtep_hit;
        }
        key = {
            meta.l3_metadata.vrf                    : exact;
            hdr.ipv4.srcAddr                        : exact;
            meta.tunnel_metadata.ingress_tunnel_type: exact;
        }
        size = 1024;
    }
    apply {
        switch (ipv4_src_vtep.apply().action_run) {
            src_vtep_hit: {
                ipv4_dest_vtep.apply();
            }
        }

    }
}

control process_ipv6_vtep(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_tunnel_termination_flag") action set_tunnel_termination_flag() {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".set_tunnel_vni_and_termination_flag") action set_tunnel_vni_and_termination_flag(bit<24> tunnel_vni) {
        meta.tunnel_metadata.tunnel_vni = tunnel_vni;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".on_miss") action on_miss() {
    }
    @name(".src_vtep_hit") action src_vtep_hit(bit<16> ifindex) {
        meta.ingress_metadata.ifindex = ifindex;
    }
    @name(".ipv6_dest_vtep") table ipv6_dest_vtep {
        actions = {
            nop;
            set_tunnel_termination_flag;
            set_tunnel_vni_and_termination_flag;
        }
        key = {
            meta.l3_metadata.vrf                    : exact;
            hdr.ipv6.dstAddr                        : exact;
            meta.tunnel_metadata.ingress_tunnel_type: exact;
        }
        size = 1024;
    }
    @name(".ipv6_src_vtep") table ipv6_src_vtep {
        actions = {
            on_miss;
            src_vtep_hit;
        }
        key = {
            meta.l3_metadata.vrf                    : exact;
            hdr.ipv6.srcAddr                        : exact;
            meta.tunnel_metadata.ingress_tunnel_type: exact;
        }
        size = 1024;
    }
    apply {
        switch (ipv6_src_vtep.apply().action_run) {
            src_vtep_hit: {
                ipv6_dest_vtep.apply();
            }
        }

    }
}

control process_mpls(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".terminate_eompls") action terminate_eompls(bit<16> bd, bit<5> tunnel_type) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type;
        meta.ingress_metadata.bd = bd;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_vpls") action terminate_vpls(bit<16> bd, bit<5> tunnel_type) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type;
        meta.ingress_metadata.bd = bd;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_ipv4_over_mpls") action terminate_ipv4_over_mpls(bit<16> vrf, bit<5> tunnel_type) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type;
        meta.l3_metadata.vrf = vrf;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
    }
    @name(".terminate_ipv6_over_mpls") action terminate_ipv6_over_mpls(bit<16> vrf, bit<5> tunnel_type) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type;
        meta.l3_metadata.vrf = vrf;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv6.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv6.trafficClass;
    }
    @name(".terminate_pw") action terminate_pw(bit<16> ifindex) {
        meta.ingress_metadata.egress_ifindex = ifindex;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
    }
    @name(".forward_mpls") action forward_mpls(bit<16> nexthop_index) {
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
        meta.l3_metadata.fib_hit = 1w1;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
    }
    @name(".mpls") table mpls_0 {
        actions = {
            terminate_eompls;
            terminate_vpls;
            terminate_ipv4_over_mpls;
            terminate_ipv6_over_mpls;
            terminate_pw;
            forward_mpls;
        }
        key = {
            meta.tunnel_metadata.mpls_label: exact;
            hdr.inner_ipv4.isValid()       : exact;
            hdr.inner_ipv6.isValid()       : exact;
        }
        size = 1024;
    }
    apply {
        mpls_0.apply();
    }
}

control process_tunnel(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss() {
    }
    @name(".outer_rmac_hit") action outer_rmac_hit() {
        meta.l3_metadata.rmac_hit = 1w1;
    }
    @name(".nop") action nop() {
    }
    @name(".tunnel_lookup_miss") action tunnel_lookup_miss() {
    }
    @name(".terminate_tunnel_inner_non_ip") action terminate_tunnel_inner_non_ip(bit<16> bd, bit<16> bd_label, bit<16> stats_idx) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.ingress_metadata.bd = bd;
        meta.acl_metadata.bd_label = bd_label;
        meta.l2_metadata.bd_stats_idx = stats_idx;
        meta.l3_metadata.lkp_ip_type = 2w0;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_tunnel_inner_ethernet_ipv4") action terminate_tunnel_inner_ethernet_ipv4(bit<16> bd, bit<16> vrf, bit<10> rmac_group, bit<16> bd_label, bit<1> ipv4_unicast_enabled, bit<2> ipv4_urpf_mode, bit<1> igmp_snooping_enabled, bit<16> stats_idx, bit<1> ipv4_multicast_enabled, bit<16> mrpf_group) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.ingress_metadata.bd = bd;
        meta.l3_metadata.vrf = vrf;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.ipv4_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled;
        meta.ipv4_metadata.ipv4_urpf_mode = ipv4_urpf_mode;
        meta.l3_metadata.rmac_group = rmac_group;
        meta.acl_metadata.bd_label = bd_label;
        meta.l2_metadata.bd_stats_idx = stats_idx;
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
        meta.multicast_metadata.igmp_snooping_enabled = igmp_snooping_enabled;
        meta.multicast_metadata.ipv4_multicast_enabled = ipv4_multicast_enabled;
        meta.multicast_metadata.bd_mrpf_group = mrpf_group;
    }
    @name(".terminate_tunnel_inner_ipv4") action terminate_tunnel_inner_ipv4(bit<16> vrf, bit<10> rmac_group, bit<2> ipv4_urpf_mode, bit<1> ipv4_unicast_enabled, bit<1> ipv4_multicast_enabled, bit<16> mrpf_group) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.l3_metadata.vrf = vrf;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.ipv4_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled;
        meta.ipv4_metadata.ipv4_urpf_mode = ipv4_urpf_mode;
        meta.l3_metadata.rmac_group = rmac_group;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
        meta.multicast_metadata.bd_mrpf_group = mrpf_group;
        meta.multicast_metadata.ipv4_multicast_enabled = ipv4_multicast_enabled;
    }
    @name(".terminate_tunnel_inner_ethernet_ipv6") action terminate_tunnel_inner_ethernet_ipv6(bit<16> bd, bit<16> vrf, bit<10> rmac_group, bit<16> bd_label, bit<1> ipv6_unicast_enabled, bit<2> ipv6_urpf_mode, bit<1> mld_snooping_enabled, bit<16> stats_idx, bit<1> ipv6_multicast_enabled, bit<16> mrpf_group) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.ingress_metadata.bd = bd;
        meta.l3_metadata.vrf = vrf;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.ipv6_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled;
        meta.ipv6_metadata.ipv6_urpf_mode = ipv6_urpf_mode;
        meta.l3_metadata.rmac_group = rmac_group;
        meta.acl_metadata.bd_label = bd_label;
        meta.l2_metadata.bd_stats_idx = stats_idx;
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv6.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv6.trafficClass;
        meta.multicast_metadata.bd_mrpf_group = mrpf_group;
        meta.multicast_metadata.ipv6_multicast_enabled = ipv6_multicast_enabled;
        meta.multicast_metadata.mld_snooping_enabled = mld_snooping_enabled;
    }
    @name(".terminate_tunnel_inner_ipv6") action terminate_tunnel_inner_ipv6(bit<16> vrf, bit<10> rmac_group, bit<1> ipv6_unicast_enabled, bit<2> ipv6_urpf_mode, bit<1> ipv6_multicast_enabled, bit<16> mrpf_group) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.l3_metadata.vrf = vrf;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.ipv6_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled;
        meta.ipv6_metadata.ipv6_urpf_mode = ipv6_urpf_mode;
        meta.l3_metadata.rmac_group = rmac_group;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv6.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv6.trafficClass;
        meta.multicast_metadata.bd_mrpf_group = mrpf_group;
        meta.multicast_metadata.ipv6_multicast_enabled = ipv6_multicast_enabled;
    }
    @name(".non_ip_tunnel_lookup_miss") action non_ip_tunnel_lookup_miss() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        standard_metadata.mcast_grp = 16w0;
    }
    @name(".ipv4_tunnel_lookup_miss") action ipv4_tunnel_lookup_miss() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.ipv4.protocol;
        meta.l3_metadata.lkp_ip_ttl = hdr.ipv4.ttl;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_outer_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_outer_l4_dport;
        standard_metadata.mcast_grp = 16w0;
    }
    @name(".ipv6_tunnel_lookup_miss") action ipv6_tunnel_lookup_miss() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.ipv6.nextHdr;
        meta.l3_metadata.lkp_ip_ttl = hdr.ipv6.hopLimit;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_outer_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_outer_l4_dport;
        standard_metadata.mcast_grp = 16w0;
    }
    @name(".outer_rmac") table outer_rmac {
        actions = {
            on_miss;
            outer_rmac_hit;
        }
        key = {
            meta.l3_metadata.rmac_group: exact;
            hdr.ethernet.dstAddr       : exact;
        }
        size = 1024;
    }
    @name(".tunnel") table tunnel {
        actions = {
            nop;
            tunnel_lookup_miss;
            terminate_tunnel_inner_non_ip;
            terminate_tunnel_inner_ethernet_ipv4;
            terminate_tunnel_inner_ipv4;
            terminate_tunnel_inner_ethernet_ipv6;
            terminate_tunnel_inner_ipv6;
        }
        key = {
            meta.tunnel_metadata.tunnel_vni         : exact;
            meta.tunnel_metadata.ingress_tunnel_type: exact;
            hdr.inner_ipv4.isValid()                : exact;
            hdr.inner_ipv6.isValid()                : exact;
        }
        size = 1024;
    }
    @name(".tunnel_lookup_miss") table tunnel_lookup_miss_0 {
        actions = {
            non_ip_tunnel_lookup_miss;
            ipv4_tunnel_lookup_miss;
            ipv6_tunnel_lookup_miss;
        }
        key = {
            hdr.ipv4.isValid(): exact;
            hdr.ipv6.isValid(): exact;
        }
    }
    @name(".tunnel_miss") table tunnel_miss {
        actions = {
            non_ip_tunnel_lookup_miss;
            ipv4_tunnel_lookup_miss;
            ipv6_tunnel_lookup_miss;
        }
        key = {
            hdr.ipv4.isValid(): exact;
            hdr.ipv6.isValid(): exact;
        }
    }
    @name(".process_ingress_fabric") process_ingress_fabric() process_ingress_fabric_0;
    @name(".process_outer_multicast") process_outer_multicast() process_outer_multicast_0;
    @name(".process_ipv4_vtep") process_ipv4_vtep() process_ipv4_vtep_0;
    @name(".process_ipv6_vtep") process_ipv6_vtep() process_ipv6_vtep_0;
    @name(".process_mpls") process_mpls() process_mpls_0;
    apply {
        process_ingress_fabric_0.apply(hdr, meta, standard_metadata);
        if (meta.tunnel_metadata.ingress_tunnel_type != 5w0) {
            switch (outer_rmac.apply().action_run) {
                on_miss: {
                    process_outer_multicast_0.apply(hdr, meta, standard_metadata);
                }
                default: {
                    if (hdr.ipv4.isValid()) {
                        process_ipv4_vtep_0.apply(hdr, meta, standard_metadata);
                    } else {
                        if (hdr.ipv6.isValid()) {
                            process_ipv6_vtep_0.apply(hdr, meta, standard_metadata);
                        } else {
                            if (hdr.mpls[0].isValid()) {
                                process_mpls_0.apply(hdr, meta, standard_metadata);
                            }
                        }
                    }
                }
            }

        }
        if (meta.tunnel_metadata.tunnel_terminate == 1w1 || meta.multicast_metadata.outer_mcast_route_hit == 1w1 && (meta.multicast_metadata.outer_mcast_mode == 2w1 && meta.multicast_metadata.mcast_rpf_group == 16w0 || meta.multicast_metadata.outer_mcast_mode == 2w2 && meta.multicast_metadata.mcast_rpf_group != 16w0)) {
            switch (tunnel.apply().action_run) {
                tunnel_lookup_miss: {
                    tunnel_lookup_miss_0.apply();
                }
            }

        } else {
            tunnel_miss.apply();
        }
    }
}

control process_ingress_sflow(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".sflow_ingress_session_pkt_counter") direct_counter(CounterType.packets) sflow_ingress_session_pkt_counter;
    @name(".nop") action nop() {
    }
    @name(".sflow_ing_pkt_to_cpu") action sflow_ing_pkt_to_cpu(bit<32> sflow_i2e_mirror_id, bit<16> reason_code) {
        meta.fabric_metadata.reason_code = reason_code;
        meta.i2e_metadata.mirror_session_id = (bit<16>)sflow_i2e_mirror_id;
        clone3(CloneType.I2E, (bit<32>)sflow_i2e_mirror_id, { { meta.ingress_metadata.bd, meta.ingress_metadata.ifindex, meta.fabric_metadata.reason_code, meta.ingress_metadata.ingress_port }, meta.sflow_metadata.sflow_session_id, meta.i2e_metadata.mirror_session_id });
    }
    @name(".sflow_ing_session_enable") action sflow_ing_session_enable(bit<32> rate_thr, bit<16> session_id) {
        meta.ingress_metadata.sflow_take_sample = rate_thr |+| meta.ingress_metadata.sflow_take_sample;
        meta.sflow_metadata.sflow_session_id = session_id;
    }
    @name(".nop") action nop_1() {
        sflow_ingress_session_pkt_counter.count();
    }
    @name(".sflow_ing_pkt_to_cpu") action sflow_ing_pkt_to_cpu_0(bit<32> sflow_i2e_mirror_id, bit<16> reason_code) {
        sflow_ingress_session_pkt_counter.count();
        meta.fabric_metadata.reason_code = reason_code;
        meta.i2e_metadata.mirror_session_id = (bit<16>)sflow_i2e_mirror_id;
        clone3(CloneType.I2E, (bit<32>)sflow_i2e_mirror_id, { { meta.ingress_metadata.bd, meta.ingress_metadata.ifindex, meta.fabric_metadata.reason_code, meta.ingress_metadata.ingress_port }, meta.sflow_metadata.sflow_session_id, meta.i2e_metadata.mirror_session_id });
    }
    @name(".sflow_ing_take_sample") table sflow_ing_take_sample {
        actions = {
            nop_1;
            sflow_ing_pkt_to_cpu_0;
        }
        key = {
            meta.ingress_metadata.sflow_take_sample: ternary;
            meta.sflow_metadata.sflow_session_id   : exact;
        }
        counters = sflow_ingress_session_pkt_counter;
    }
    @name(".sflow_ingress") table sflow_ingress {
        actions = {
            nop;
            sflow_ing_session_enable;
        }
        key = {
            meta.ingress_metadata.ifindex : ternary;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary;
            meta.ipv4_metadata.lkp_ipv4_da: ternary;
            hdr.sflow.isValid()           : exact;
        }
        size = 512;
    }
    apply {
        sflow_ingress.apply();
        sflow_ing_take_sample.apply();
    }
}

@name(".storm_control_meter") meter(32w1024, MeterType.bytes) storm_control_meter;

control process_storm_control(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_storm_control_meter") action set_storm_control_meter(bit<32> meter_idx) {
        storm_control_meter.execute_meter((bit<32>)meter_idx, meta.meter_metadata.meter_color);
        meta.meter_metadata.meter_index = (bit<16>)meter_idx;
    }
    @name(".storm_control") table storm_control {
        actions = {
            nop;
            set_storm_control_meter;
        }
        key = {
            standard_metadata.ingress_port: exact;
            meta.l2_metadata.lkp_pkt_type : ternary;
        }
        size = 512;
    }
    apply {
        if (meta.ingress_metadata.port_type == 2w0) {
            storm_control.apply();
        }
    }
}

control process_validate_packet(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_unicast") action set_unicast() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
    }
    @name(".set_unicast_and_ipv6_src_is_link_local") action set_unicast_and_ipv6_src_is_link_local() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.ipv6_metadata.ipv6_src_is_link_local = 1w1;
    }
    @name(".set_multicast") action set_multicast() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w1;
    }
    @name(".set_multicast_and_ipv6_src_is_link_local") action set_multicast_and_ipv6_src_is_link_local() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.ipv6_metadata.ipv6_src_is_link_local = 1w1;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w1;
    }
    @name(".set_broadcast") action set_broadcast() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w2;
    }
    @name(".set_malformed_packet") action set_malformed_packet(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
    }
    @name(".validate_packet") table validate_packet {
        actions = {
            nop;
            set_unicast;
            set_unicast_and_ipv6_src_is_link_local;
            set_multicast;
            set_multicast_and_ipv6_src_is_link_local;
            set_broadcast;
            set_malformed_packet;
        }
        key = {
            meta.l2_metadata.lkp_mac_sa[40:40]     : ternary @name("l2_metadata.lkp_mac_sa") ;
            meta.l2_metadata.lkp_mac_da            : ternary;
            meta.l3_metadata.lkp_ip_type           : ternary;
            meta.l3_metadata.lkp_ip_ttl            : ternary;
            meta.l3_metadata.lkp_ip_version        : ternary;
            meta.ipv4_metadata.lkp_ipv4_sa[31:24]  : ternary @name("ipv4_metadata.lkp_ipv4_sa") ;
            meta.ipv6_metadata.lkp_ipv6_sa[127:112]: ternary @name("ipv6_metadata.lkp_ipv6_sa") ;
        }
        size = 512;
    }
    apply {
        if (meta.ingress_metadata.drop_flag == 1w0) {
            validate_packet.apply();
        }
    }
}

control process_mac(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".dmac_hit") action dmac_hit(bit<16> ifindex) {
        meta.ingress_metadata.egress_ifindex = ifindex;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex;
    }
    @name(".dmac_multicast_hit") action dmac_multicast_hit(bit<16> mc_index) {
        standard_metadata.mcast_grp = mc_index;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".dmac_miss") action dmac_miss() {
        meta.ingress_metadata.egress_ifindex = 16w65535;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".dmac_redirect_nexthop") action dmac_redirect_nexthop(bit<16> nexthop_index) {
        meta.l2_metadata.l2_redirect = 1w1;
        meta.l2_metadata.l2_nexthop = nexthop_index;
        meta.l2_metadata.l2_nexthop_type = 1w0;
    }
    @name(".dmac_redirect_ecmp") action dmac_redirect_ecmp(bit<16> ecmp_index) {
        meta.l2_metadata.l2_redirect = 1w1;
        meta.l2_metadata.l2_nexthop = ecmp_index;
        meta.l2_metadata.l2_nexthop_type = 1w1;
    }
    @name(".dmac_drop") action dmac_drop() {
        mark_to_drop(standard_metadata);
    }
    @name(".smac_miss") action smac_miss() {
        meta.l2_metadata.l2_src_miss = 1w1;
    }
    @name(".smac_hit") action smac_hit(bit<16> ifindex) {
        meta.l2_metadata.l2_src_move = meta.ingress_metadata.ifindex ^ ifindex;
    }
    @name(".dmac") table dmac {
        support_timeout = true;
        actions = {
            nop;
            dmac_hit;
            dmac_multicast_hit;
            dmac_miss;
            dmac_redirect_nexthop;
            dmac_redirect_ecmp;
            dmac_drop;
        }
        key = {
            meta.ingress_metadata.bd   : exact;
            meta.l2_metadata.lkp_mac_da: exact;
        }
        size = 1024;
    }
    @name(".smac") table smac {
        actions = {
            nop;
            smac_miss;
            smac_hit;
        }
        key = {
            meta.ingress_metadata.bd   : exact;
            meta.l2_metadata.lkp_mac_sa: exact;
        }
        size = 1024;
    }
    apply {
        if (meta.ingress_metadata.port_type == 2w0) {
            smac.apply();
        }
        if (meta.ingress_metadata.bypass_lookups & 16w0x1 == 16w0) {
            dmac.apply();
        }
    }
}

control process_mac_acl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".acl_deny") action acl_deny(bit<14> acl_stats_index, bit<16> acl_meter_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.meter_metadata.meter_index = acl_meter_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".acl_permit") action acl_permit(bit<14> acl_stats_index, bit<16> acl_meter_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.meter_metadata.meter_index = acl_meter_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".acl_mirror") action acl_mirror(bit<32> session_id, bit<14> acl_stats_index, bit<16> acl_meter_index) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)standard_metadata.ingress_global_timestamp;
        clone3(CloneType.I2E, (bit<32>)session_id, { meta.i2e_metadata.ingress_tstamp, meta.i2e_metadata.mirror_session_id });
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.meter_metadata.meter_index = acl_meter_index;
    }
    @name(".acl_redirect_nexthop") action acl_redirect_nexthop(bit<16> nexthop_index, bit<14> acl_stats_index, bit<16> acl_meter_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = nexthop_index;
        meta.acl_metadata.acl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.meter_metadata.meter_index = acl_meter_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".acl_redirect_ecmp") action acl_redirect_ecmp(bit<16> ecmp_index, bit<14> acl_stats_index, bit<16> acl_meter_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = ecmp_index;
        meta.acl_metadata.acl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.meter_metadata.meter_index = acl_meter_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".mac_acl") table mac_acl {
        actions = {
            nop;
            acl_deny;
            acl_permit;
            acl_mirror;
            acl_redirect_nexthop;
            acl_redirect_ecmp;
        }
        key = {
            meta.acl_metadata.if_label   : ternary;
            meta.acl_metadata.bd_label   : ternary;
            meta.l2_metadata.lkp_mac_sa  : ternary;
            meta.l2_metadata.lkp_mac_da  : ternary;
            meta.l2_metadata.lkp_mac_type: ternary;
        }
        size = 512;
    }
    apply {
        if (meta.ingress_metadata.bypass_lookups & 16w0x4 == 16w0) {
            mac_acl.apply();
        }
    }
}

control process_ip_acl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".acl_deny") action acl_deny(bit<14> acl_stats_index, bit<16> acl_meter_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.meter_metadata.meter_index = acl_meter_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".acl_permit") action acl_permit(bit<14> acl_stats_index, bit<16> acl_meter_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.meter_metadata.meter_index = acl_meter_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".acl_mirror") action acl_mirror(bit<32> session_id, bit<14> acl_stats_index, bit<16> acl_meter_index) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)standard_metadata.ingress_global_timestamp;
        clone3(CloneType.I2E, (bit<32>)session_id, { meta.i2e_metadata.ingress_tstamp, meta.i2e_metadata.mirror_session_id });
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.meter_metadata.meter_index = acl_meter_index;
    }
    @name(".acl_redirect_nexthop") action acl_redirect_nexthop(bit<16> nexthop_index, bit<14> acl_stats_index, bit<16> acl_meter_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = nexthop_index;
        meta.acl_metadata.acl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.meter_metadata.meter_index = acl_meter_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".acl_redirect_ecmp") action acl_redirect_ecmp(bit<16> ecmp_index, bit<14> acl_stats_index, bit<16> acl_meter_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = ecmp_index;
        meta.acl_metadata.acl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.meter_metadata.meter_index = acl_meter_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".ip_acl") table ip_acl {
        actions = {
            nop;
            acl_deny;
            acl_permit;
            acl_mirror;
            acl_redirect_nexthop;
            acl_redirect_ecmp;
        }
        key = {
            meta.acl_metadata.if_label    : ternary;
            meta.acl_metadata.bd_label    : ternary;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary;
            meta.ipv4_metadata.lkp_ipv4_da: ternary;
            meta.l3_metadata.lkp_ip_proto : ternary;
            meta.l3_metadata.lkp_l4_sport : ternary;
            meta.l3_metadata.lkp_l4_dport : ternary;
            hdr.tcp.flags                 : ternary;
            meta.l3_metadata.lkp_ip_ttl   : ternary;
        }
        size = 512;
    }
    @name(".ipv6_acl") table ipv6_acl {
        actions = {
            nop;
            acl_deny;
            acl_permit;
            acl_mirror;
            acl_redirect_nexthop;
            acl_redirect_ecmp;
        }
        key = {
            meta.acl_metadata.if_label    : ternary;
            meta.acl_metadata.bd_label    : ternary;
            meta.ipv6_metadata.lkp_ipv6_sa: ternary;
            meta.ipv6_metadata.lkp_ipv6_da: ternary;
            meta.l3_metadata.lkp_ip_proto : ternary;
            meta.l3_metadata.lkp_l4_sport : ternary;
            meta.l3_metadata.lkp_l4_dport : ternary;
            hdr.tcp.flags                 : ternary;
            meta.l3_metadata.lkp_ip_ttl   : ternary;
        }
        size = 512;
    }
    apply {
        if (meta.ingress_metadata.bypass_lookups & 16w0x4 == 16w0) {
            if (meta.l3_metadata.lkp_ip_type == 2w1) {
                ip_acl.apply();
            } else {
                if (meta.l3_metadata.lkp_ip_type == 2w2) {
                    ipv6_acl.apply();
                }
            }
        }
    }
}

control process_qos(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".apply_cos_marking") action apply_cos_marking(bit<3> cos) {
        meta.qos_metadata.marked_cos = cos;
    }
    @name(".apply_dscp_marking") action apply_dscp_marking(bit<8> dscp) {
        meta.qos_metadata.marked_dscp = dscp;
    }
    @name(".apply_tc_marking") action apply_tc_marking(bit<3> tc) {
        meta.qos_metadata.marked_exp = tc;
    }
    @name(".qos") table qos {
        actions = {
            nop;
            apply_cos_marking;
            apply_dscp_marking;
            apply_tc_marking;
        }
        key = {
            meta.acl_metadata.if_label    : ternary;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary;
            meta.ipv4_metadata.lkp_ipv4_da: ternary;
            meta.l3_metadata.lkp_ip_proto : ternary;
            meta.l3_metadata.lkp_ip_tc    : ternary;
            meta.tunnel_metadata.mpls_exp : ternary;
            meta.qos_metadata.outer_dscp  : ternary;
        }
        size = 512;
    }
    apply {
        qos.apply();
    }
}

control process_ipv4_multicast(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".ipv4_multicast_route_s_g_stats") direct_counter(CounterType.packets) ipv4_multicast_route_s_g_stats;
    @name(".ipv4_multicast_route_star_g_stats") direct_counter(CounterType.packets) ipv4_multicast_route_star_g_stats;
    @name(".on_miss") action on_miss() {
    }
    @name(".multicast_bridge_s_g_hit") action multicast_bridge_s_g_hit(bit<16> mc_index) {
        meta.multicast_metadata.multicast_bridge_mc_index = mc_index;
        meta.multicast_metadata.mcast_bridge_hit = 1w1;
    }
    @name(".nop") action nop() {
    }
    @name(".multicast_bridge_star_g_hit") action multicast_bridge_star_g_hit(bit<16> mc_index) {
        meta.multicast_metadata.multicast_bridge_mc_index = mc_index;
        meta.multicast_metadata.mcast_bridge_hit = 1w1;
    }
    @name(".multicast_route_s_g_hit") action multicast_route_s_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".multicast_route_star_g_miss") action multicast_route_star_g_miss() {
        meta.l3_metadata.l3_copy = 1w1;
    }
    @name(".multicast_route_sm_star_g_hit") action multicast_route_sm_star_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".multicast_route_bidir_star_g_hit") action multicast_route_bidir_star_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        meta.multicast_metadata.mcast_mode = 2w2;
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group | meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".ipv4_multicast_bridge") table ipv4_multicast_bridge {
        actions = {
            on_miss;
            multicast_bridge_s_g_hit;
        }
        key = {
            meta.ingress_metadata.bd      : exact;
            meta.ipv4_metadata.lkp_ipv4_sa: exact;
            meta.ipv4_metadata.lkp_ipv4_da: exact;
        }
        size = 1024;
    }
    @name(".ipv4_multicast_bridge_star_g") table ipv4_multicast_bridge_star_g {
        actions = {
            nop;
            multicast_bridge_star_g_hit;
        }
        key = {
            meta.ingress_metadata.bd      : exact;
            meta.ipv4_metadata.lkp_ipv4_da: exact;
        }
        size = 1024;
    }
    @name(".on_miss") action on_miss_0() {
        ipv4_multicast_route_s_g_stats.count();
    }
    @name(".multicast_route_s_g_hit") action multicast_route_s_g_hit_0(bit<16> mc_index, bit<16> mcast_rpf_group) {
        ipv4_multicast_route_s_g_stats.count();
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".ipv4_multicast_route") table ipv4_multicast_route {
        actions = {
            on_miss_0;
            multicast_route_s_g_hit_0;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv4_metadata.lkp_ipv4_sa: exact;
            meta.ipv4_metadata.lkp_ipv4_da: exact;
        }
        size = 1024;
        counters = ipv4_multicast_route_s_g_stats;
    }
    @name(".multicast_route_star_g_miss") action multicast_route_star_g_miss_0() {
        ipv4_multicast_route_star_g_stats.count();
        meta.l3_metadata.l3_copy = 1w1;
    }
    @name(".multicast_route_sm_star_g_hit") action multicast_route_sm_star_g_hit_0(bit<16> mc_index, bit<16> mcast_rpf_group) {
        ipv4_multicast_route_star_g_stats.count();
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".multicast_route_bidir_star_g_hit") action multicast_route_bidir_star_g_hit_0(bit<16> mc_index, bit<16> mcast_rpf_group) {
        ipv4_multicast_route_star_g_stats.count();
        meta.multicast_metadata.mcast_mode = 2w2;
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group | meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".ipv4_multicast_route_star_g") table ipv4_multicast_route_star_g {
        actions = {
            multicast_route_star_g_miss_0;
            multicast_route_sm_star_g_hit_0;
            multicast_route_bidir_star_g_hit_0;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv4_metadata.lkp_ipv4_da: exact;
        }
        size = 1024;
        counters = ipv4_multicast_route_star_g_stats;
    }
    apply {
        if (meta.ingress_metadata.bypass_lookups & 16w0x1 == 16w0) {
            switch (ipv4_multicast_bridge.apply().action_run) {
                on_miss: {
                    ipv4_multicast_bridge_star_g.apply();
                }
            }

        }
        if (meta.ingress_metadata.bypass_lookups & 16w0x2 == 16w0 && meta.multicast_metadata.ipv4_multicast_enabled == 1w1) {
            switch (ipv4_multicast_route.apply().action_run) {
                on_miss_0: {
                    ipv4_multicast_route_star_g.apply();
                }
            }

        }
    }
}

control process_ipv6_multicast(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".ipv6_multicast_route_s_g_stats") direct_counter(CounterType.packets) ipv6_multicast_route_s_g_stats;
    @name(".ipv6_multicast_route_star_g_stats") direct_counter(CounterType.packets) ipv6_multicast_route_star_g_stats;
    @name(".on_miss") action on_miss() {
    }
    @name(".multicast_bridge_s_g_hit") action multicast_bridge_s_g_hit(bit<16> mc_index) {
        meta.multicast_metadata.multicast_bridge_mc_index = mc_index;
        meta.multicast_metadata.mcast_bridge_hit = 1w1;
    }
    @name(".nop") action nop() {
    }
    @name(".multicast_bridge_star_g_hit") action multicast_bridge_star_g_hit(bit<16> mc_index) {
        meta.multicast_metadata.multicast_bridge_mc_index = mc_index;
        meta.multicast_metadata.mcast_bridge_hit = 1w1;
    }
    @name(".multicast_route_s_g_hit") action multicast_route_s_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".multicast_route_star_g_miss") action multicast_route_star_g_miss() {
        meta.l3_metadata.l3_copy = 1w1;
    }
    @name(".multicast_route_sm_star_g_hit") action multicast_route_sm_star_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".multicast_route_bidir_star_g_hit") action multicast_route_bidir_star_g_hit(bit<16> mc_index, bit<16> mcast_rpf_group) {
        meta.multicast_metadata.mcast_mode = 2w2;
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group | meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".ipv6_multicast_bridge") table ipv6_multicast_bridge {
        actions = {
            on_miss;
            multicast_bridge_s_g_hit;
        }
        key = {
            meta.ingress_metadata.bd      : exact;
            meta.ipv6_metadata.lkp_ipv6_sa: exact;
            meta.ipv6_metadata.lkp_ipv6_da: exact;
        }
        size = 1024;
    }
    @name(".ipv6_multicast_bridge_star_g") table ipv6_multicast_bridge_star_g {
        actions = {
            nop;
            multicast_bridge_star_g_hit;
        }
        key = {
            meta.ingress_metadata.bd      : exact;
            meta.ipv6_metadata.lkp_ipv6_da: exact;
        }
        size = 1024;
    }
    @name(".on_miss") action on_miss_1() {
        ipv6_multicast_route_s_g_stats.count();
    }
    @name(".multicast_route_s_g_hit") action multicast_route_s_g_hit_1(bit<16> mc_index, bit<16> mcast_rpf_group) {
        ipv6_multicast_route_s_g_stats.count();
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".ipv6_multicast_route") table ipv6_multicast_route {
        actions = {
            on_miss_1;
            multicast_route_s_g_hit_1;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv6_metadata.lkp_ipv6_sa: exact;
            meta.ipv6_metadata.lkp_ipv6_da: exact;
        }
        size = 1024;
        counters = ipv6_multicast_route_s_g_stats;
    }
    @name(".multicast_route_star_g_miss") action multicast_route_star_g_miss_1() {
        ipv6_multicast_route_star_g_stats.count();
        meta.l3_metadata.l3_copy = 1w1;
    }
    @name(".multicast_route_sm_star_g_hit") action multicast_route_sm_star_g_hit_1(bit<16> mc_index, bit<16> mcast_rpf_group) {
        ipv6_multicast_route_star_g_stats.count();
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".multicast_route_bidir_star_g_hit") action multicast_route_bidir_star_g_hit_1(bit<16> mc_index, bit<16> mcast_rpf_group) {
        ipv6_multicast_route_star_g_stats.count();
        meta.multicast_metadata.mcast_mode = 2w2;
        meta.multicast_metadata.multicast_route_mc_index = mc_index;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group | meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".ipv6_multicast_route_star_g") table ipv6_multicast_route_star_g {
        actions = {
            multicast_route_star_g_miss_1;
            multicast_route_sm_star_g_hit_1;
            multicast_route_bidir_star_g_hit_1;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv6_metadata.lkp_ipv6_da: exact;
        }
        size = 1024;
        counters = ipv6_multicast_route_star_g_stats;
    }
    apply {
        if (meta.ingress_metadata.bypass_lookups & 16w0x1 == 16w0) {
            switch (ipv6_multicast_bridge.apply().action_run) {
                on_miss: {
                    ipv6_multicast_bridge_star_g.apply();
                }
            }

        }
        if (meta.ingress_metadata.bypass_lookups & 16w0x2 == 16w0 && meta.multicast_metadata.ipv6_multicast_enabled == 1w1) {
            switch (ipv6_multicast_route.apply().action_run) {
                on_miss_1: {
                    ipv6_multicast_route_star_g.apply();
                }
            }

        }
    }
}

control process_multicast_rpf(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control process_multicast(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".process_ipv4_multicast") process_ipv4_multicast() process_ipv4_multicast_0;
    @name(".process_ipv6_multicast") process_ipv6_multicast() process_ipv6_multicast_0;
    @name(".process_multicast_rpf") process_multicast_rpf() process_multicast_rpf_0;
    apply {
        if (meta.l3_metadata.lkp_ip_type == 2w1) {
            process_ipv4_multicast_0.apply(hdr, meta, standard_metadata);
        } else {
            if (meta.l3_metadata.lkp_ip_type == 2w2) {
                process_ipv6_multicast_0.apply(hdr, meta, standard_metadata);
            }
        }
        process_multicast_rpf_0.apply(hdr, meta, standard_metadata);
    }
}

control process_ipv4_racl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".racl_deny") action racl_deny(bit<14> acl_stats_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.racl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".racl_permit") action racl_permit(bit<14> acl_stats_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".racl_redirect_nexthop") action racl_redirect_nexthop(bit<16> nexthop_index, bit<14> acl_stats_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = nexthop_index;
        meta.acl_metadata.racl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".racl_redirect_ecmp") action racl_redirect_ecmp(bit<16> ecmp_index, bit<14> acl_stats_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = ecmp_index;
        meta.acl_metadata.racl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".ipv4_racl") table ipv4_racl {
        actions = {
            nop;
            racl_deny;
            racl_permit;
            racl_redirect_nexthop;
            racl_redirect_ecmp;
        }
        key = {
            meta.acl_metadata.bd_label    : ternary;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary;
            meta.ipv4_metadata.lkp_ipv4_da: ternary;
            meta.l3_metadata.lkp_ip_proto : ternary;
            meta.l3_metadata.lkp_l4_sport : ternary;
            meta.l3_metadata.lkp_l4_dport : ternary;
        }
        size = 512;
    }
    apply {
        ipv4_racl.apply();
    }
}

control process_ipv4_urpf(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss() {
    }
    @name(".ipv4_urpf_hit") action ipv4_urpf_hit(bit<16> urpf_bd_group) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group;
        meta.l3_metadata.urpf_mode = meta.ipv4_metadata.ipv4_urpf_mode;
    }
    @name(".urpf_miss") action urpf_miss() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".ipv4_urpf") table ipv4_urpf {
        actions = {
            on_miss;
            ipv4_urpf_hit;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv4_metadata.lkp_ipv4_sa: exact;
        }
        size = 1024;
    }
    @name(".ipv4_urpf_lpm") table ipv4_urpf_lpm {
        actions = {
            ipv4_urpf_hit;
            urpf_miss;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv4_metadata.lkp_ipv4_sa: lpm;
        }
        size = 512;
    }
    apply {
        if (meta.ipv4_metadata.ipv4_urpf_mode != 2w0) {
            switch (ipv4_urpf.apply().action_run) {
                on_miss: {
                    ipv4_urpf_lpm.apply();
                }
            }

        }
    }
}

control process_ipv4_fib(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss() {
    }
    @name(".fib_hit_nexthop") action fib_hit_nexthop(bit<16> nexthop_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_ecmp") action fib_hit_ecmp(bit<16> ecmp_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".ipv4_fib") table ipv4_fib {
        actions = {
            on_miss;
            fib_hit_nexthop;
            fib_hit_ecmp;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv4_metadata.lkp_ipv4_da: exact;
        }
        size = 1024;
    }
    @name(".ipv4_fib_lpm") table ipv4_fib_lpm {
        actions = {
            on_miss;
            fib_hit_nexthop;
            fib_hit_ecmp;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv4_metadata.lkp_ipv4_da: lpm;
        }
        size = 512;
    }
    apply {
        switch (ipv4_fib.apply().action_run) {
            on_miss: {
                ipv4_fib_lpm.apply();
            }
        }

    }
}

control process_ipv6_racl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".racl_deny") action racl_deny(bit<14> acl_stats_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.racl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".racl_permit") action racl_permit(bit<14> acl_stats_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".racl_redirect_nexthop") action racl_redirect_nexthop(bit<16> nexthop_index, bit<14> acl_stats_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = nexthop_index;
        meta.acl_metadata.racl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".racl_redirect_ecmp") action racl_redirect_ecmp(bit<16> ecmp_index, bit<14> acl_stats_index, bit<1> acl_copy, bit<16> acl_copy_reason) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = ecmp_index;
        meta.acl_metadata.racl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
        meta.acl_metadata.acl_copy = acl_copy;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".ipv6_racl") table ipv6_racl {
        actions = {
            nop;
            racl_deny;
            racl_permit;
            racl_redirect_nexthop;
            racl_redirect_ecmp;
        }
        key = {
            meta.acl_metadata.bd_label    : ternary;
            meta.ipv6_metadata.lkp_ipv6_sa: ternary;
            meta.ipv6_metadata.lkp_ipv6_da: ternary;
            meta.l3_metadata.lkp_ip_proto : ternary;
            meta.l3_metadata.lkp_l4_sport : ternary;
            meta.l3_metadata.lkp_l4_dport : ternary;
        }
        size = 512;
    }
    apply {
        ipv6_racl.apply();
    }
}

control process_ipv6_urpf(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss() {
    }
    @name(".ipv6_urpf_hit") action ipv6_urpf_hit(bit<16> urpf_bd_group) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group;
        meta.l3_metadata.urpf_mode = meta.ipv6_metadata.ipv6_urpf_mode;
    }
    @name(".urpf_miss") action urpf_miss() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".ipv6_urpf") table ipv6_urpf {
        actions = {
            on_miss;
            ipv6_urpf_hit;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv6_metadata.lkp_ipv6_sa: exact;
        }
        size = 1024;
    }
    @name(".ipv6_urpf_lpm") table ipv6_urpf_lpm {
        actions = {
            ipv6_urpf_hit;
            urpf_miss;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv6_metadata.lkp_ipv6_sa: lpm;
        }
        size = 512;
    }
    apply {
        if (meta.ipv6_metadata.ipv6_urpf_mode != 2w0) {
            switch (ipv6_urpf.apply().action_run) {
                on_miss: {
                    ipv6_urpf_lpm.apply();
                }
            }

        }
    }
}

control process_ipv6_fib(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss() {
    }
    @name(".fib_hit_nexthop") action fib_hit_nexthop(bit<16> nexthop_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_ecmp") action fib_hit_ecmp(bit<16> ecmp_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".ipv6_fib") table ipv6_fib {
        actions = {
            on_miss;
            fib_hit_nexthop;
            fib_hit_ecmp;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv6_metadata.lkp_ipv6_da: exact;
        }
        size = 1024;
    }
    @name(".ipv6_fib_lpm") table ipv6_fib_lpm {
        actions = {
            on_miss;
            fib_hit_nexthop;
            fib_hit_ecmp;
        }
        key = {
            meta.l3_metadata.vrf          : exact;
            meta.ipv6_metadata.lkp_ipv6_da: lpm;
        }
        size = 512;
    }
    apply {
        switch (ipv6_fib.apply().action_run) {
            on_miss: {
                ipv6_fib_lpm.apply();
            }
        }

    }
}

control process_urpf_bd(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".urpf_bd_miss") action urpf_bd_miss() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".urpf_bd") table urpf_bd {
        actions = {
            nop;
            urpf_bd_miss;
        }
        key = {
            meta.l3_metadata.urpf_bd_group: exact;
            meta.ingress_metadata.bd      : exact;
        }
        size = 1024;
    }
    apply {
        if (meta.l3_metadata.urpf_mode == 2w2 && meta.l3_metadata.urpf_hit == 1w1) {
            urpf_bd.apply();
        }
    }
}

control process_meter_index(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".meter_index") direct_meter<bit<2>>(MeterType.bytes) meter_index;
    @name(".nop") action nop() {
    }
    @name(".nop") action nop_2() {
        meter_index.read(meta.meter_metadata.meter_color);
    }
    @name(".meter_index") table meter_index_0 {
        actions = {
            nop_2;
        }
        key = {
            meta.meter_metadata.meter_index: exact;
        }
        size = 1024;
        meters = meter_index;
    }
    apply {
        if (meta.ingress_metadata.bypass_lookups & 16w0x10 == 16w0) {
            meter_index_0.apply();
        }
    }
}

control process_hashes(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".compute_lkp_ipv4_hash") action compute_lkp_ipv4_hash() {
        hash(meta.hash_metadata.hash1, HashAlgorithm.crc16, (bit<16>)0, { meta.ipv4_metadata.lkp_ipv4_sa, meta.ipv4_metadata.lkp_ipv4_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, (bit<32>)65536);
        hash(meta.hash_metadata.hash2, HashAlgorithm.crc16, (bit<16>)0, { meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.ipv4_metadata.lkp_ipv4_sa, meta.ipv4_metadata.lkp_ipv4_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, (bit<32>)65536);
    }
    @name(".compute_lkp_ipv6_hash") action compute_lkp_ipv6_hash() {
        hash(meta.hash_metadata.hash1, HashAlgorithm.crc16, (bit<16>)0, { meta.ipv6_metadata.lkp_ipv6_sa, meta.ipv6_metadata.lkp_ipv6_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, (bit<32>)65536);
        hash(meta.hash_metadata.hash2, HashAlgorithm.crc16, (bit<16>)0, { meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.ipv6_metadata.lkp_ipv6_sa, meta.ipv6_metadata.lkp_ipv6_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, (bit<32>)65536);
    }
    @name(".compute_lkp_non_ip_hash") action compute_lkp_non_ip_hash() {
        hash(meta.hash_metadata.hash2, HashAlgorithm.crc16, (bit<16>)0, { meta.ingress_metadata.ifindex, meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.l2_metadata.lkp_mac_type }, (bit<32>)65536);
    }
    @name(".computed_two_hashes") action computed_two_hashes() {
        meta.hash_metadata.entropy_hash = meta.hash_metadata.hash2;
    }
    @name(".computed_one_hash") action computed_one_hash() {
        meta.hash_metadata.hash1 = meta.hash_metadata.hash2;
        meta.hash_metadata.entropy_hash = meta.hash_metadata.hash2;
    }
    @name(".compute_ipv4_hashes") table compute_ipv4_hashes {
        actions = {
            compute_lkp_ipv4_hash;
        }
        key = {
            meta.ingress_metadata.drop_flag: exact;
        }
    }
    @name(".compute_ipv6_hashes") table compute_ipv6_hashes {
        actions = {
            compute_lkp_ipv6_hash;
        }
        key = {
            meta.ingress_metadata.drop_flag: exact;
        }
    }
    @name(".compute_non_ip_hashes") table compute_non_ip_hashes {
        actions = {
            compute_lkp_non_ip_hash;
        }
        key = {
            meta.ingress_metadata.drop_flag: exact;
        }
    }
    @name(".compute_other_hashes") table compute_other_hashes {
        actions = {
            computed_two_hashes;
            computed_one_hash;
        }
        key = {
            meta.hash_metadata.hash1: exact;
        }
    }
    apply {
        if (meta.tunnel_metadata.tunnel_terminate == 1w0 && hdr.ipv4.isValid() || meta.tunnel_metadata.tunnel_terminate == 1w1 && hdr.inner_ipv4.isValid()) {
            compute_ipv4_hashes.apply();
        } else {
            if (meta.tunnel_metadata.tunnel_terminate == 1w0 && hdr.ipv6.isValid() || meta.tunnel_metadata.tunnel_terminate == 1w1 && hdr.inner_ipv6.isValid()) {
                compute_ipv6_hashes.apply();
            } else {
                compute_non_ip_hashes.apply();
            }
        }
        compute_other_hashes.apply();
    }
}

control process_meter_action(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".meter_stats") direct_counter(CounterType.packets) meter_stats;
    @name(".meter_permit") action meter_permit() {
    }
    @name(".meter_deny") action meter_deny() {
        mark_to_drop(standard_metadata);
    }
    @name(".meter_permit") action meter_permit_0() {
        meter_stats.count();
    }
    @name(".meter_deny") action meter_deny_0() {
        meter_stats.count();
        mark_to_drop(standard_metadata);
    }
    @name(".meter_action") table meter_action {
        actions = {
            meter_permit_0;
            meter_deny_0;
        }
        key = {
            meta.meter_metadata.meter_color: exact;
            meta.meter_metadata.meter_index: exact;
        }
        size = 1024;
        counters = meter_stats;
    }
    apply {
        if (meta.ingress_metadata.bypass_lookups & 16w0x10 == 16w0) {
            meter_action.apply();
        }
    }
}

@name(".ingress_bd_stats_count") @min_width(32) counter(32w1024, CounterType.packets_and_bytes) ingress_bd_stats_count;

control process_ingress_bd_stats(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".update_ingress_bd_stats") action update_ingress_bd_stats() {
        ingress_bd_stats_count.count((bit<32>)(bit<32>)meta.l2_metadata.bd_stats_idx);
    }
    @name(".ingress_bd_stats") table ingress_bd_stats {
        actions = {
            update_ingress_bd_stats;
        }
        size = 1024;
    }
    apply {
        ingress_bd_stats.apply();
    }
}

@name(".acl_stats_count") @min_width(16) counter(32w1024, CounterType.packets_and_bytes) acl_stats_count;

control process_ingress_acl_stats(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".acl_stats_update") action acl_stats_update() {
        acl_stats_count.count((bit<32>)(bit<32>)meta.acl_metadata.acl_stats_index);
    }
    @name(".acl_stats") table acl_stats {
        actions = {
            acl_stats_update;
        }
        size = 1024;
    }
    apply {
        acl_stats.apply();
    }
}

control process_storm_control_stats(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".storm_control_stats") direct_counter(CounterType.packets) storm_control_stats;
    @name(".nop") action nop() {
    }
    @name(".nop") action nop_3() {
        storm_control_stats.count();
    }
    @name(".storm_control_stats") table storm_control_stats_0 {
        actions = {
            nop_3;
        }
        key = {
            meta.meter_metadata.meter_color: exact;
            standard_metadata.ingress_port : exact;
        }
        size = 1024;
        counters = storm_control_stats;
    }
    apply {
        storm_control_stats_0.apply();
    }
}

control process_fwd_results(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_l2_redirect_action") action set_l2_redirect_action() {
        meta.l3_metadata.nexthop_index = meta.l2_metadata.l2_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.l2_metadata.l2_nexthop_type;
        meta.ingress_metadata.egress_ifindex = 16w0;
        standard_metadata.mcast_grp = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_fib_redirect_action") action set_fib_redirect_action() {
        meta.l3_metadata.nexthop_index = meta.l3_metadata.fib_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.l3_metadata.fib_nexthop_type;
        meta.l3_metadata.routed = 1w1;
        standard_metadata.mcast_grp = 16w0;
        meta.fabric_metadata.reason_code = 16w0x217;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_cpu_redirect_action") action set_cpu_redirect_action() {
        meta.l3_metadata.routed = 1w0;
        standard_metadata.mcast_grp = 16w0;
        standard_metadata.egress_spec = 9w64;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_acl_redirect_action") action set_acl_redirect_action() {
        meta.l3_metadata.nexthop_index = meta.acl_metadata.acl_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.acl_metadata.acl_nexthop_type;
        meta.ingress_metadata.egress_ifindex = 16w0;
        standard_metadata.mcast_grp = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_racl_redirect_action") action set_racl_redirect_action() {
        meta.l3_metadata.nexthop_index = meta.acl_metadata.racl_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.acl_metadata.racl_nexthop_type;
        meta.l3_metadata.routed = 1w1;
        meta.ingress_metadata.egress_ifindex = 16w0;
        standard_metadata.mcast_grp = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_multicast_route_action") action set_multicast_route_action() {
        meta.fabric_metadata.dst_device = 8w127;
        meta.ingress_metadata.egress_ifindex = 16w0;
        standard_metadata.mcast_grp = meta.multicast_metadata.multicast_route_mc_index;
        meta.l3_metadata.routed = 1w1;
        meta.l3_metadata.same_bd_check = 16w0xffff;
    }
    @name(".set_multicast_bridge_action") action set_multicast_bridge_action() {
        meta.fabric_metadata.dst_device = 8w127;
        meta.ingress_metadata.egress_ifindex = 16w0;
        standard_metadata.mcast_grp = meta.multicast_metadata.multicast_bridge_mc_index;
    }
    @name(".set_multicast_flood") action set_multicast_flood() {
        meta.fabric_metadata.dst_device = 8w127;
        meta.ingress_metadata.egress_ifindex = 16w65535;
    }
    @name(".set_multicast_drop") action set_multicast_drop() {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = 8w44;
    }
    @name(".fwd_result") table fwd_result {
        actions = {
            nop;
            set_l2_redirect_action;
            set_fib_redirect_action;
            set_cpu_redirect_action;
            set_acl_redirect_action;
            set_racl_redirect_action;
            set_multicast_route_action;
            set_multicast_bridge_action;
            set_multicast_flood;
            set_multicast_drop;
        }
        key = {
            meta.l2_metadata.l2_redirect                 : ternary;
            meta.acl_metadata.acl_redirect               : ternary;
            meta.acl_metadata.racl_redirect              : ternary;
            meta.l3_metadata.rmac_hit                    : ternary;
            meta.l3_metadata.fib_hit                     : ternary;
            meta.l2_metadata.lkp_pkt_type                : ternary;
            meta.l3_metadata.lkp_ip_type                 : ternary;
            meta.multicast_metadata.igmp_snooping_enabled: ternary;
            meta.multicast_metadata.mld_snooping_enabled : ternary;
            meta.multicast_metadata.mcast_route_hit      : ternary;
            meta.multicast_metadata.mcast_bridge_hit     : ternary;
            meta.multicast_metadata.mcast_rpf_group      : ternary;
            meta.multicast_metadata.mcast_mode           : ternary;
        }
        size = 512;
    }
    apply {
        if (!(meta.ingress_metadata.bypass_lookups == 16w0xffff)) {
            fwd_result.apply();
        }
    }
}

control process_nexthop(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_ecmp_nexthop_details") action set_ecmp_nexthop_details(bit<16> ifindex, bit<16> bd, bit<16> nhop_index, bit<1> tunnel) {
        meta.ingress_metadata.egress_ifindex = ifindex;
        meta.l3_metadata.nexthop_index = nhop_index;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex;
        meta.tunnel_metadata.tunnel_if_check = meta.tunnel_metadata.tunnel_terminate ^ tunnel;
    }
    @name(".set_ecmp_nexthop_details_for_post_routed_flood") action set_ecmp_nexthop_details_for_post_routed_flood(bit<16> bd, bit<16> uuc_mc_index, bit<16> nhop_index) {
        standard_metadata.mcast_grp = uuc_mc_index;
        meta.l3_metadata.nexthop_index = nhop_index;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".set_nexthop_details") action set_nexthop_details(bit<16> ifindex, bit<16> bd, bit<1> tunnel) {
        meta.ingress_metadata.egress_ifindex = ifindex;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex;
        meta.tunnel_metadata.tunnel_if_check = meta.tunnel_metadata.tunnel_terminate ^ tunnel;
    }
    @name(".set_nexthop_details_for_post_routed_flood") action set_nexthop_details_for_post_routed_flood(bit<16> bd, bit<16> uuc_mc_index) {
        standard_metadata.mcast_grp = uuc_mc_index;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".ecmp_group") table ecmp_group {
        actions = {
            nop;
            set_ecmp_nexthop_details;
            set_ecmp_nexthop_details_for_post_routed_flood;
        }
        key = {
            meta.l3_metadata.nexthop_index: exact;
            meta.hash_metadata.hash1      : selector;
        }
        size = 1024;
        implementation = ecmp_action_profile;
    }
    @name(".nexthop") table nexthop {
        actions = {
            nop;
            set_nexthop_details;
            set_nexthop_details_for_post_routed_flood;
        }
        key = {
            meta.l3_metadata.nexthop_index: exact;
        }
        size = 1024;
    }
    apply {
        if (meta.nexthop_metadata.nexthop_type == 1w1) {
            ecmp_group.apply();
        } else {
            nexthop.apply();
        }
    }
}

control process_multicast_flooding(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_bd_flood_mc_index") action set_bd_flood_mc_index(bit<16> mc_index) {
        standard_metadata.mcast_grp = mc_index;
    }
    @name(".bd_flood") table bd_flood {
        actions = {
            nop;
            set_bd_flood_mc_index;
        }
        key = {
            meta.ingress_metadata.bd     : exact;
            meta.l2_metadata.lkp_pkt_type: exact;
        }
        size = 1024;
    }
    apply {
        bd_flood.apply();
    }
}

control process_lag(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_lag_miss") action set_lag_miss() {
    }
    @name(".set_lag_port") action set_lag_port(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".set_lag_remote_port") action set_lag_remote_port(bit<8> device, bit<16> port) {
        meta.fabric_metadata.dst_device = device;
        meta.fabric_metadata.dst_port = port;
    }
    @name(".lag_group") table lag_group {
        actions = {
            set_lag_miss;
            set_lag_port;
            set_lag_remote_port;
        }
        key = {
            meta.ingress_metadata.egress_ifindex: exact;
            meta.hash_metadata.hash2            : selector;
        }
        size = 1024;
        implementation = lag_action_profile;
    }
    apply {
        lag_group.apply();
    }
}

@name("mac_learn_digest") struct mac_learn_digest {
    bit<16> bd;
    bit<48> lkp_mac_sa;
    bit<16> ifindex;
}

control process_mac_learning(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".generate_learn_notify") action generate_learn_notify() {
        digest<mac_learn_digest>((bit<32>)1024, { meta.ingress_metadata.bd, meta.l2_metadata.lkp_mac_sa, meta.ingress_metadata.ifindex });
    }
    @name(".learn_notify") table learn_notify {
        actions = {
            nop;
            generate_learn_notify;
        }
        key = {
            meta.l2_metadata.l2_src_miss: ternary;
            meta.l2_metadata.l2_src_move: ternary;
            meta.l2_metadata.stp_state  : ternary;
        }
        size = 512;
    }
    apply {
        if (meta.l2_metadata.learning_enabled == 1w1) {
            learn_notify.apply();
        }
    }
}

control process_fabric_lag(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_fabric_lag_port") action set_fabric_lag_port(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".set_fabric_multicast") action set_fabric_multicast(bit<8> fabric_mgid) {
        meta.multicast_metadata.mcast_grp = standard_metadata.mcast_grp;
    }
    @name(".fabric_lag") table fabric_lag {
        actions = {
            nop;
            set_fabric_lag_port;
            set_fabric_multicast;
        }
        key = {
            meta.fabric_metadata.dst_device: exact;
            meta.hash_metadata.hash2       : selector;
        }
        implementation = fabric_lag_action_profile;
    }
    apply {
        fabric_lag.apply();
    }
}

@name(".drop_stats") counter(32w1024, CounterType.packets) drop_stats;

@name(".drop_stats_2") counter(32w1024, CounterType.packets) drop_stats_2;

control process_system_acl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".drop_stats_update") action drop_stats_update() {
        drop_stats_2.count((bit<32>)(bit<32>)meta.ingress_metadata.drop_reason);
    }
    @name(".nop") action nop() {
    }
    @name(".copy_to_cpu_with_reason") action copy_to_cpu_with_reason(bit<16> reason_code) {
        meta.fabric_metadata.reason_code = reason_code;
        clone3(CloneType.I2E, (bit<32>)32w250, { meta.ingress_metadata.bd, meta.ingress_metadata.ifindex, meta.fabric_metadata.reason_code, meta.ingress_metadata.ingress_port });
    }
    @name(".redirect_to_cpu") action redirect_to_cpu(bit<16> reason_code) {
        copy_to_cpu_with_reason(reason_code);
        mark_to_drop(standard_metadata);
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".copy_to_cpu") action copy_to_cpu() {
        clone3(CloneType.I2E, (bit<32>)32w250, { meta.ingress_metadata.bd, meta.ingress_metadata.ifindex, meta.fabric_metadata.reason_code, meta.ingress_metadata.ingress_port });
    }
    @name(".drop_packet") action drop_packet() {
        mark_to_drop(standard_metadata);
    }
    @name(".drop_packet_with_reason") action drop_packet_with_reason(bit<32> drop_reason) {
        drop_stats.count((bit<32>)drop_reason);
        mark_to_drop(standard_metadata);
    }
    @name(".negative_mirror") action negative_mirror(bit<32> session_id) {
        clone3(CloneType.I2E, (bit<32>)session_id, { meta.ingress_metadata.ifindex, meta.ingress_metadata.drop_reason });
        mark_to_drop(standard_metadata);
    }
    @name(".drop_stats") table drop_stats_0 {
        actions = {
            drop_stats_update;
        }
        size = 1024;
    }
    @name(".system_acl") table system_acl {
        actions = {
            nop;
            redirect_to_cpu;
            copy_to_cpu_with_reason;
            copy_to_cpu;
            drop_packet;
            drop_packet_with_reason;
            negative_mirror;
        }
        key = {
            meta.acl_metadata.if_label                : ternary;
            meta.acl_metadata.bd_label                : ternary;
            meta.l2_metadata.lkp_mac_sa               : ternary;
            meta.l2_metadata.lkp_mac_da               : ternary;
            meta.l2_metadata.lkp_mac_type             : ternary;
            meta.ingress_metadata.ifindex             : ternary;
            meta.l2_metadata.port_vlan_mapping_miss   : ternary;
            meta.security_metadata.ipsg_check_fail    : ternary;
            meta.security_metadata.storm_control_color: ternary;
            meta.acl_metadata.acl_deny                : ternary;
            meta.acl_metadata.racl_deny               : ternary;
            meta.l3_metadata.urpf_check_fail          : ternary;
            meta.ingress_metadata.drop_flag           : ternary;
            meta.acl_metadata.acl_copy                : ternary;
            meta.l3_metadata.l3_copy                  : ternary;
            meta.l3_metadata.rmac_hit                 : ternary;
            meta.l3_metadata.routed                   : ternary;
            meta.ipv6_metadata.ipv6_src_is_link_local : ternary;
            meta.l2_metadata.same_if_check            : ternary;
            meta.tunnel_metadata.tunnel_if_check      : ternary;
            meta.l3_metadata.same_bd_check            : ternary;
            meta.l3_metadata.lkp_ip_ttl               : ternary;
            meta.l2_metadata.stp_state                : ternary;
            meta.ingress_metadata.control_frame       : ternary;
            meta.ipv4_metadata.ipv4_unicast_enabled   : ternary;
            meta.ipv6_metadata.ipv6_unicast_enabled   : ternary;
            meta.ingress_metadata.egress_ifindex      : ternary;
        }
        size = 512;
    }
    apply {
        if (meta.ingress_metadata.bypass_lookups & 16w0x20 == 16w0) {
            system_acl.apply();
            if (meta.ingress_metadata.drop_flag == 1w1) {
                drop_stats_0.apply();
            }
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".rmac_hit") action rmac_hit() {
        meta.l3_metadata.rmac_hit = 1w1;
    }
    @name(".rmac_miss") action rmac_miss() {
        meta.l3_metadata.rmac_hit = 1w0;
    }
    @name(".rmac") table rmac {
        actions = {
            rmac_hit;
            rmac_miss;
        }
        key = {
            meta.l3_metadata.rmac_group: exact;
            meta.l2_metadata.lkp_mac_da: exact;
        }
        size = 1024;
    }
    @name(".process_ingress_port_mapping") process_ingress_port_mapping() process_ingress_port_mapping_0;
    @name(".process_validate_outer_header") process_validate_outer_header() process_validate_outer_header_0;
    @name(".process_global_params") process_global_params() process_global_params_0;
    @name(".process_port_vlan_mapping") process_port_vlan_mapping() process_port_vlan_mapping_0;
    @name(".process_spanning_tree") process_spanning_tree() process_spanning_tree_0;
    @name(".process_ip_sourceguard") process_ip_sourceguard() process_ip_sourceguard_0;
    @name(".process_int_endpoint") process_int_endpoint() process_int_endpoint_0;
    @name(".process_tunnel") process_tunnel() process_tunnel_0;
    @name(".process_ingress_sflow") process_ingress_sflow() process_ingress_sflow_0;
    @name(".process_storm_control") process_storm_control() process_storm_control_0;
    @name(".process_validate_packet") process_validate_packet() process_validate_packet_0;
    @name(".process_mac") process_mac() process_mac_0;
    @name(".process_mac_acl") process_mac_acl() process_mac_acl_0;
    @name(".process_ip_acl") process_ip_acl() process_ip_acl_0;
    @name(".process_qos") process_qos() process_qos_0;
    @name(".process_multicast") process_multicast() process_multicast_0;
    @name(".process_ipv4_racl") process_ipv4_racl() process_ipv4_racl_0;
    @name(".process_ipv4_urpf") process_ipv4_urpf() process_ipv4_urpf_0;
    @name(".process_ipv4_fib") process_ipv4_fib() process_ipv4_fib_0;
    @name(".process_ipv6_racl") process_ipv6_racl() process_ipv6_racl_0;
    @name(".process_ipv6_urpf") process_ipv6_urpf() process_ipv6_urpf_0;
    @name(".process_ipv6_fib") process_ipv6_fib() process_ipv6_fib_0;
    @name(".process_urpf_bd") process_urpf_bd() process_urpf_bd_0;
    @name(".process_meter_index") process_meter_index() process_meter_index_0;
    @name(".process_hashes") process_hashes() process_hashes_0;
    @name(".process_meter_action") process_meter_action() process_meter_action_0;
    @name(".process_ingress_bd_stats") process_ingress_bd_stats() process_ingress_bd_stats_0;
    @name(".process_ingress_acl_stats") process_ingress_acl_stats() process_ingress_acl_stats_0;
    @name(".process_storm_control_stats") process_storm_control_stats() process_storm_control_stats_0;
    @name(".process_fwd_results") process_fwd_results() process_fwd_results_0;
    @name(".process_nexthop") process_nexthop() process_nexthop_0;
    @name(".process_multicast_flooding") process_multicast_flooding() process_multicast_flooding_0;
    @name(".process_lag") process_lag() process_lag_0;
    @name(".process_mac_learning") process_mac_learning() process_mac_learning_0;
    @name(".process_fabric_lag") process_fabric_lag() process_fabric_lag_0;
    @name(".process_system_acl") process_system_acl() process_system_acl_0;
    apply {
        process_ingress_port_mapping_0.apply(hdr, meta, standard_metadata);
        process_validate_outer_header_0.apply(hdr, meta, standard_metadata);
        process_global_params_0.apply(hdr, meta, standard_metadata);
        process_port_vlan_mapping_0.apply(hdr, meta, standard_metadata);
        process_spanning_tree_0.apply(hdr, meta, standard_metadata);
        process_ip_sourceguard_0.apply(hdr, meta, standard_metadata);
        process_int_endpoint_0.apply(hdr, meta, standard_metadata);
        process_tunnel_0.apply(hdr, meta, standard_metadata);
        process_ingress_sflow_0.apply(hdr, meta, standard_metadata);
        process_storm_control_0.apply(hdr, meta, standard_metadata);
        if (meta.ingress_metadata.port_type != 2w1) {
            if (!(hdr.mpls[0].isValid() && meta.l3_metadata.fib_hit == 1w1)) {
                process_validate_packet_0.apply(hdr, meta, standard_metadata);
                process_mac_0.apply(hdr, meta, standard_metadata);
                if (meta.l3_metadata.lkp_ip_type == 2w0) {
                    process_mac_acl_0.apply(hdr, meta, standard_metadata);
                } else {
                    process_ip_acl_0.apply(hdr, meta, standard_metadata);
                }
                process_qos_0.apply(hdr, meta, standard_metadata);
                switch (rmac.apply().action_run) {
                    rmac_miss: {
                        process_multicast_0.apply(hdr, meta, standard_metadata);
                    }
                    default: {
                        if (meta.ingress_metadata.bypass_lookups & 16w0x2 == 16w0) {
                            if (meta.l3_metadata.lkp_ip_type == 2w1 && meta.ipv4_metadata.ipv4_unicast_enabled == 1w1) {
                                process_ipv4_racl_0.apply(hdr, meta, standard_metadata);
                                process_ipv4_urpf_0.apply(hdr, meta, standard_metadata);
                                process_ipv4_fib_0.apply(hdr, meta, standard_metadata);
                            } else {
                                if (meta.l3_metadata.lkp_ip_type == 2w2 && meta.ipv6_metadata.ipv6_unicast_enabled == 1w1) {
                                    process_ipv6_racl_0.apply(hdr, meta, standard_metadata);
                                    process_ipv6_urpf_0.apply(hdr, meta, standard_metadata);
                                    process_ipv6_fib_0.apply(hdr, meta, standard_metadata);
                                }
                            }
                            process_urpf_bd_0.apply(hdr, meta, standard_metadata);
                        }
                    }
                }

            }
        }
        process_meter_index_0.apply(hdr, meta, standard_metadata);
        process_hashes_0.apply(hdr, meta, standard_metadata);
        process_meter_action_0.apply(hdr, meta, standard_metadata);
        if (meta.ingress_metadata.port_type != 2w1) {
            process_ingress_bd_stats_0.apply(hdr, meta, standard_metadata);
            process_ingress_acl_stats_0.apply(hdr, meta, standard_metadata);
            process_storm_control_stats_0.apply(hdr, meta, standard_metadata);
            process_fwd_results_0.apply(hdr, meta, standard_metadata);
            process_nexthop_0.apply(hdr, meta, standard_metadata);
            if (meta.ingress_metadata.egress_ifindex == 16w65535) {
                process_multicast_flooding_0.apply(hdr, meta, standard_metadata);
            } else {
                process_lag_0.apply(hdr, meta, standard_metadata);
            }
            process_mac_learning_0.apply(hdr, meta, standard_metadata);
        }
        process_fabric_lag_0.apply(hdr, meta, standard_metadata);
        if (meta.ingress_metadata.port_type != 2w1) {
            process_system_acl_0.apply(hdr, meta, standard_metadata);
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.fabric_header);
        packet.emit(hdr.fabric_header_cpu);
        packet.emit(hdr.fabric_header_sflow);
        packet.emit(hdr.fabric_header_mirror);
        packet.emit(hdr.fabric_header_multicast);
        packet.emit(hdr.fabric_header_unicast);
        packet.emit(hdr.fabric_payload_header);
        packet.emit(hdr.llc_header);
        packet.emit(hdr.snap_header);
        packet.emit(hdr.vlan_tag_[0]);
        packet.emit(hdr.vlan_tag_[1]);
        packet.emit(hdr.arp_rarp);
        packet.emit(hdr.arp_rarp_ipv4);
        packet.emit(hdr.ipv6);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.gre);
        packet.emit(hdr.erspan_t3_header);
        packet.emit(hdr.nvgre);
        packet.emit(hdr.udp);
        packet.emit(hdr.sflow);
        packet.emit(hdr.vxlan_gpe);
        packet.emit(hdr.vxlan_gpe_int_header);
        packet.emit(hdr.int_header);
        packet.emit(hdr.int_switch_id_header);
        packet.emit(hdr.int_ingress_port_id_header);
        packet.emit(hdr.int_hop_latency_header);
        packet.emit(hdr.int_q_occupancy_header);
        packet.emit(hdr.int_ingress_tstamp_header);
        packet.emit(hdr.int_egress_port_id_header);
        packet.emit(hdr.int_q_congestion_header);
        packet.emit(hdr.int_egress_port_tx_utilization_header);
        packet.emit(hdr.int_val);
        packet.emit(hdr.genv);
        packet.emit(hdr.vxlan);
        packet.emit(hdr.tcp);
        packet.emit(hdr.icmp);
        packet.emit(hdr.mpls);
        packet.emit(hdr.inner_ethernet);
        packet.emit(hdr.inner_ipv6);
        packet.emit(hdr.inner_ipv4);
        packet.emit(hdr.inner_udp);
        packet.emit(hdr.inner_tcp);
        packet.emit(hdr.inner_icmp);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum(hdr.inner_ipv4.ihl == 4w5, { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr }, hdr.inner_ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum(hdr.inner_ipv4.ihl == 4w5, { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr }, hdr.inner_ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

