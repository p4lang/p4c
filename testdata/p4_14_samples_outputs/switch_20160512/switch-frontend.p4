#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

enum bit<8> FieldLists {
    none = 8w0,
    i2e_mirror_info = 8w1,
    cpu_info = 8w2,
    e2e_mirror_info = 8w3,
    int_i2e_mirror_info = 8w4,
    mirror_info = 8w5,
    sflow_cpu_info = 8w6
}

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
    @field_list(FieldLists.cpu_info, FieldLists.sflow_cpu_info)
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
    @field_list(FieldLists.i2e_mirror_info, FieldLists.e2e_mirror_info)
    bit<32> ingress_tstamp;
    @field_list(FieldLists.i2e_mirror_info, FieldLists.e2e_mirror_info, FieldLists.int_i2e_mirror_info)
    bit<16> mirror_session_id;
}

struct ingress_metadata_t {
    @field_list(FieldLists.cpu_info, FieldLists.sflow_cpu_info)
    bit<9>  ingress_port;
    @field_list(FieldLists.cpu_info, FieldLists.mirror_info, FieldLists.sflow_cpu_info)
    bit<16> ifindex;
    bit<16> egress_ifindex;
    bit<2>  port_type;
    bit<16> outer_bd;
    @field_list(FieldLists.cpu_info, FieldLists.sflow_cpu_info)
    bit<16> bd;
    bit<1>  drop_flag;
    @field_list(FieldLists.mirror_info)
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
    @field_list(FieldLists.int_i2e_mirror_info)
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
    @name("ParserImpl.tmp") bit<4> tmp;
    @name("ParserImpl.tmp_0") bit<4> tmp_0;
    @name(".parse_all_int_meta_value_heders") state parse_all_int_meta_value_heders {
        packet.extract<int_switch_id_header_t>(hdr.int_switch_id_header);
        packet.extract<int_ingress_port_id_header_t>(hdr.int_ingress_port_id_header);
        packet.extract<int_hop_latency_header_t>(hdr.int_hop_latency_header);
        packet.extract<int_q_occupancy_header_t>(hdr.int_q_occupancy_header);
        packet.extract<int_ingress_tstamp_header_t>(hdr.int_ingress_tstamp_header);
        packet.extract<int_egress_port_id_header_t>(hdr.int_egress_port_id_header);
        packet.extract<int_q_congestion_header_t>(hdr.int_q_congestion_header);
        packet.extract<int_egress_port_tx_utilization_header_t>(hdr.int_egress_port_tx_utilization_header);
        transition parse_int_val;
    }
    @name(".parse_arp_rarp") state parse_arp_rarp {
        packet.extract<arp_rarp_t>(hdr.arp_rarp);
        transition select(hdr.arp_rarp.protoType) {
            16w0x800: parse_arp_rarp_ipv4;
            default: accept;
        }
    }
    @name(".parse_arp_rarp_ipv4") state parse_arp_rarp_ipv4 {
        packet.extract<arp_rarp_ipv4_t>(hdr.arp_rarp_ipv4);
        transition parse_set_prio_med;
    }
    @name(".parse_eompls") state parse_eompls {
        meta.tunnel_metadata.ingress_tunnel_type = 5w6;
        transition parse_inner_ethernet;
    }
    @name(".parse_erspan_t3") state parse_erspan_t3 {
        packet.extract<erspan_header_t3_t_0>(hdr.erspan_t3_header);
        transition parse_inner_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
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
        packet.extract<fabric_header_t>(hdr.fabric_header);
        transition select(hdr.fabric_header.packetType) {
            3w1: parse_fabric_header_unicast;
            3w2: parse_fabric_header_multicast;
            3w3: parse_fabric_header_mirror;
            3w5: parse_fabric_header_cpu;
            default: accept;
        }
    }
    @name(".parse_fabric_header_cpu") state parse_fabric_header_cpu {
        packet.extract<fabric_header_cpu_t>(hdr.fabric_header_cpu);
        meta.ingress_metadata.bypass_lookups = hdr.fabric_header_cpu.reasonCode;
        transition select(hdr.fabric_header_cpu.reasonCode) {
            16w0x4: parse_fabric_sflow_header;
            default: parse_fabric_payload_header;
        }
    }
    @name(".parse_fabric_header_mirror") state parse_fabric_header_mirror {
        packet.extract<fabric_header_mirror_t>(hdr.fabric_header_mirror);
        transition parse_fabric_payload_header;
    }
    @name(".parse_fabric_header_multicast") state parse_fabric_header_multicast {
        packet.extract<fabric_header_multicast_t>(hdr.fabric_header_multicast);
        transition parse_fabric_payload_header;
    }
    @name(".parse_fabric_header_unicast") state parse_fabric_header_unicast {
        packet.extract<fabric_header_unicast_t>(hdr.fabric_header_unicast);
        transition parse_fabric_payload_header;
    }
    @name(".parse_fabric_payload_header") state parse_fabric_payload_header {
        packet.extract<fabric_payload_header_t>(hdr.fabric_payload_header);
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
        packet.extract<fabric_header_sflow_t>(hdr.fabric_header_sflow);
        transition parse_fabric_payload_header;
    }
    @name(".parse_geneve") state parse_geneve {
        packet.extract<genv_t>(hdr.genv);
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
        packet.extract<vxlan_gpe_int_header_t>(hdr.vxlan_gpe_int_header);
        meta.int_metadata.gpe_int_hdr_len = (bit<16>)hdr.vxlan_gpe_int_header.len;
        transition parse_int_header;
    }
    @name(".parse_gre") state parse_gre {
        packet.extract<gre_t>(hdr.gre);
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
    @name(".parse_icmp") state parse_icmp {
        packet.extract<icmp_t>(hdr.icmp);
        meta.l3_metadata.lkp_outer_l4_sport = hdr.icmp.typeCode;
        transition select(hdr.icmp.typeCode) {
            16w0x8200 &&& 16w0xfe00: parse_set_prio_med;
            16w0x8400 &&& 16w0xfc00: parse_set_prio_med;
            16w0x8800 &&& 16w0xff00: parse_set_prio_med;
            default: accept;
        }
    }
    @name(".parse_inner_ethernet") state parse_inner_ethernet {
        packet.extract<ethernet_t>(hdr.inner_ethernet);
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        transition select(hdr.inner_ethernet.etherType) {
            16w0x800: parse_inner_ipv4;
            16w0x86dd: parse_inner_ipv6;
            default: accept;
        }
    }
    @name(".parse_inner_icmp") state parse_inner_icmp {
        packet.extract<icmp_t>(hdr.inner_icmp);
        meta.l3_metadata.lkp_l4_sport = hdr.inner_icmp.typeCode;
        transition accept;
    }
    @name(".parse_inner_ipv4") state parse_inner_ipv4 {
        packet.extract<ipv4_t>(hdr.inner_ipv4);
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
        packet.extract<ipv6_t>(hdr.inner_ipv6);
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
    @name(".parse_inner_tcp") state parse_inner_tcp {
        packet.extract<tcp_t>(hdr.inner_tcp);
        meta.l3_metadata.lkp_l4_sport = hdr.inner_tcp.srcPort;
        meta.l3_metadata.lkp_l4_dport = hdr.inner_tcp.dstPort;
        transition accept;
    }
    @name(".parse_inner_udp") state parse_inner_udp {
        packet.extract<udp_t>(hdr.inner_udp);
        meta.l3_metadata.lkp_l4_sport = hdr.inner_udp.srcPort;
        meta.l3_metadata.lkp_l4_dport = hdr.inner_udp.dstPort;
        transition accept;
    }
    @name(".parse_int_header") state parse_int_header {
        packet.extract<int_header_t>(hdr.int_header);
        meta.int_metadata.instruction_cnt = (bit<16>)hdr.int_header.ins_cnt;
        transition select(hdr.int_header.rsvd1, hdr.int_header.total_hop_cnt) {
            (5w0x0, 8w0x0): accept;
            (5w0x0 &&& 5w0xf, 8w0x0 &&& 8w0x0): parse_int_val;
            default: accept;
            default: parse_all_int_meta_value_heders;
        }
    }
    @name(".parse_int_val") state parse_int_val {
        packet.extract<int_value_t>(hdr.int_val.next);
        transition select(hdr.int_val.last.bos) {
            1w0: parse_int_val;
            1w1: parse_inner_ethernet;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
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
        packet.extract<ipv6_t>(hdr.ipv6);
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
    @name(".parse_llc_header") state parse_llc_header {
        packet.extract<llc_header_t>(hdr.llc_header);
        transition select(hdr.llc_header.dsap, hdr.llc_header.ssap) {
            (8w0xaa, 8w0xaa): parse_snap_header;
            (8w0xfe, 8w0xfe): parse_set_prio_med;
            default: accept;
        }
    }
    @name(".parse_mpls") state parse_mpls {
        packet.extract<mpls_t>(hdr.mpls.next);
        transition select(hdr.mpls.last.bos) {
            1w0: parse_mpls;
            1w1: parse_mpls_bos;
            default: accept;
        }
    }
    @name(".parse_mpls_bos") state parse_mpls_bos {
        tmp_0 = packet.lookahead<bit<4>>();
        tmp = tmp_0;
        transition select(tmp) {
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
    @name(".parse_nvgre") state parse_nvgre {
        packet.extract<nvgre_t>(hdr.nvgre);
        meta.tunnel_metadata.ingress_tunnel_type = 5w5;
        meta.tunnel_metadata.tunnel_vni = hdr.nvgre.tni;
        transition parse_inner_ethernet;
    }
    @name(".parse_qinq") state parse_qinq {
        packet.extract<vlan_tag_t>(hdr.vlan_tag_[0]);
        transition select(hdr.vlan_tag_[0].etherType) {
            16w0x8100: parse_qinq_vlan;
            default: accept;
        }
    }
    @name(".parse_qinq_vlan") state parse_qinq_vlan {
        packet.extract<vlan_tag_t>(hdr.vlan_tag_[1]);
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
    @name(".parse_set_prio_high") state parse_set_prio_high {
        standard_metadata.priority = 3w5;
        transition accept;
    }
    @name(".parse_set_prio_med") state parse_set_prio_med {
        standard_metadata.priority = 3w3;
        transition accept;
    }
    @name(".parse_sflow") state parse_sflow {
        packet.extract<sflow_hdr_t>(hdr.sflow);
        transition accept;
    }
    @name(".parse_snap_header") state parse_snap_header {
        packet.extract<snap_header_t>(hdr.snap_header);
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
        packet.extract<tcp_t>(hdr.tcp);
        meta.l3_metadata.lkp_outer_l4_sport = hdr.tcp.srcPort;
        meta.l3_metadata.lkp_outer_l4_dport = hdr.tcp.dstPort;
        transition select(hdr.tcp.dstPort) {
            16w179: parse_set_prio_med;
            16w639: parse_set_prio_med;
            default: accept;
        }
    }
    @name(".parse_udp") state parse_udp {
        packet.extract<udp_t>(hdr.udp);
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
    @name(".parse_vlan") state parse_vlan {
        packet.extract<vlan_tag_t>(hdr.vlan_tag_[0]);
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
    @name(".parse_vxlan") state parse_vxlan {
        packet.extract<vxlan_t>(hdr.vxlan);
        meta.tunnel_metadata.ingress_tunnel_type = 5w1;
        meta.tunnel_metadata.tunnel_vni = hdr.vxlan.vni;
        transition parse_inner_ethernet;
    }
    @name(".parse_vxlan_gpe") state parse_vxlan_gpe {
        packet.extract<vxlan_gpe_t>(hdr.vxlan_gpe);
        meta.tunnel_metadata.ingress_tunnel_type = 5w12;
        meta.tunnel_metadata.tunnel_vni = hdr.vxlan_gpe.vni;
        transition select(hdr.vxlan_gpe.flags, hdr.vxlan_gpe.next_proto) {
            (8w0x8 &&& 8w0x8, 8w0x5): parse_gpe_int_header;
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
control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_6() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_7() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_8() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_9() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_10() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_11() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_12() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_13() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_14() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_15() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_16() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_17() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_18() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_19() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_20() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_21() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_22() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_23() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_24() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_25() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_26() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_27() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_28() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_29() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_30() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_31() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_32() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_33() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_34() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_35() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_36() {
    }
    @name(".egress_port_type_normal") action egress_port_type_normal(@name("ifindex") bit<16> ifindex_11) {
        meta.egress_metadata.port_type = 2w0;
        meta.egress_metadata.ifindex = ifindex_11;
    }
    @name(".egress_port_type_fabric") action egress_port_type_fabric(@name("ifindex") bit<16> ifindex_12) {
        meta.egress_metadata.port_type = 2w1;
        meta.tunnel_metadata.egress_tunnel_type = 5w15;
        meta.egress_metadata.ifindex = ifindex_12;
    }
    @name(".egress_port_type_cpu") action egress_port_type_cpu(@name("ifindex") bit<16> ifindex_13) {
        meta.egress_metadata.port_type = 2w2;
        meta.tunnel_metadata.egress_tunnel_type = 5w16;
        meta.egress_metadata.ifindex = ifindex_13;
    }
    @name(".nop") action nop() {
    }
    @name(".set_mirror_nhop") action set_mirror_nhop(@name("nhop_idx") bit<16> nhop_idx) {
        meta.l3_metadata.nexthop_index = nhop_idx;
    }
    @name(".set_mirror_bd") action set_mirror_bd(@name("bd") bit<16> bd_17) {
        meta.egress_metadata.bd = bd_17;
    }
    @name(".sflow_pkt_to_cpu") action sflow_pkt_to_cpu() {
        hdr.fabric_header_sflow.setValid();
        hdr.fabric_header_sflow.sflow_session_id = meta.sflow_metadata.sflow_session_id;
    }
    @name(".egress_port_mapping") table egress_port_mapping_0 {
        actions = {
            egress_port_type_normal();
            egress_port_type_fabric();
            egress_port_type_cpu();
            @defaultonly NoAction_2();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port");
        }
        size = 288;
        default_action = NoAction_2();
    }
    @name(".mirror") table mirror_0 {
        actions = {
            nop();
            set_mirror_nhop();
            set_mirror_bd();
            sflow_pkt_to_cpu();
            @defaultonly NoAction_3();
        }
        key = {
            meta.i2e_metadata.mirror_session_id: exact @name("i2e_metadata.mirror_session_id");
        }
        size = 1024;
        default_action = NoAction_3();
    }
    @name(".nop") action _nop() {
    }
    @name(".nop") action _nop_0() {
    }
    @name(".set_replica_copy_bridged") action _set_replica_copy_bridged_0() {
        meta.egress_metadata.routed = 1w0;
    }
    @name(".outer_replica_from_rid") action _outer_replica_from_rid_0(@name("bd") bit<16> bd_18, @name("tunnel_index") bit<14> tunnel_index_9, @name("tunnel_type") bit<5> tunnel_type, @name("header_count") bit<4> header_count) {
        meta.egress_metadata.bd = bd_18;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w0;
        meta.egress_metadata.routed = meta.l3_metadata.outer_routed;
        meta.egress_metadata.same_bd_check = bd_18 ^ meta.ingress_metadata.outer_bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index_9;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
        meta.tunnel_metadata.egress_header_count = header_count;
    }
    @name(".inner_replica_from_rid") action _inner_replica_from_rid_0(@name("bd") bit<16> bd_19, @name("tunnel_index") bit<14> tunnel_index_10, @name("tunnel_type") bit<5> tunnel_type_8, @name("header_count") bit<4> header_count_6) {
        meta.egress_metadata.bd = bd_19;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w1;
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.same_bd_check = bd_19 ^ meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index_10;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type_8;
        meta.tunnel_metadata.egress_header_count = header_count_6;
    }
    @name(".replica_type") table _replica_type {
        actions = {
            _nop();
            _set_replica_copy_bridged_0();
            @defaultonly NoAction_4();
        }
        key = {
            meta.multicast_metadata.replica   : exact @name("multicast_metadata.replica");
            meta.egress_metadata.same_bd_check: ternary @name("egress_metadata.same_bd_check");
        }
        size = 512;
        default_action = NoAction_4();
    }
    @name(".rid") table _rid {
        actions = {
            _nop_0();
            _outer_replica_from_rid_0();
            _inner_replica_from_rid_0();
            @defaultonly NoAction_5();
        }
        key = {
            standard_metadata.egress_rid: exact @name("standard_metadata.egress_rid");
        }
        size = 1024;
        default_action = NoAction_5();
    }
    @name(".nop") action _nop_1() {
    }
    @name(".remove_vlan_single_tagged") action _remove_vlan_single_tagged_0() {
        hdr.ethernet.etherType = hdr.vlan_tag_[0].etherType;
        hdr.vlan_tag_[0].setInvalid();
    }
    @name(".remove_vlan_double_tagged") action _remove_vlan_double_tagged_0() {
        hdr.ethernet.etherType = hdr.vlan_tag_[1].etherType;
        hdr.vlan_tag_[0].setInvalid();
        hdr.vlan_tag_[1].setInvalid();
    }
    @name(".vlan_decap") table _vlan_decap {
        actions = {
            _nop_1();
            _remove_vlan_single_tagged_0();
            _remove_vlan_double_tagged_0();
            @defaultonly NoAction_6();
        }
        key = {
            hdr.vlan_tag_[0].isValid(): exact @name("vlan_tag_[0].$valid$");
            hdr.vlan_tag_[1].isValid(): exact @name("vlan_tag_[1].$valid$");
        }
        size = 1024;
        default_action = NoAction_6();
    }
    @name(".decap_inner_udp") action _decap_inner_udp_0() {
        hdr.udp = hdr.inner_udp;
        hdr.inner_udp.setInvalid();
    }
    @name(".decap_inner_tcp") action _decap_inner_tcp_0() {
        hdr.tcp = hdr.inner_tcp;
        hdr.inner_tcp.setInvalid();
        hdr.udp.setInvalid();
    }
    @name(".decap_inner_icmp") action _decap_inner_icmp_0() {
        hdr.icmp = hdr.inner_icmp;
        hdr.inner_icmp.setInvalid();
        hdr.udp.setInvalid();
    }
    @name(".decap_inner_unknown") action _decap_inner_unknown_0() {
        hdr.udp.setInvalid();
    }
    @name(".decap_vxlan_inner_ipv4") action _decap_vxlan_inner_ipv4_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.vxlan.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_vxlan_inner_ipv6") action _decap_vxlan_inner_ipv6_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.vxlan.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_vxlan_inner_non_ip") action _decap_vxlan_inner_non_ip_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.vxlan.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_genv_inner_ipv4") action _decap_genv_inner_ipv4_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.genv.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_genv_inner_ipv6") action _decap_genv_inner_ipv6_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.genv.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_genv_inner_non_ip") action _decap_genv_inner_non_ip_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.genv.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_nvgre_inner_ipv4") action _decap_nvgre_inner_ipv4_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_nvgre_inner_ipv6") action _decap_nvgre_inner_ipv6_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_nvgre_inner_non_ip") action _decap_nvgre_inner_non_ip_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_gre_inner_ipv4") action _decap_gre_inner_ipv4_0() {
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.gre.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_gre_inner_ipv6") action _decap_gre_inner_ipv6_0() {
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_gre_inner_non_ip") action _decap_gre_inner_non_ip_0() {
        hdr.ethernet.etherType = hdr.gre.proto;
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_ip_inner_ipv4") action _decap_ip_inner_ipv4_0() {
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.ipv6.setInvalid();
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_ip_inner_ipv6") action _decap_ip_inner_ipv6_0() {
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.ipv4.setInvalid();
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ipv4_pop1") action _decap_mpls_inner_ipv4_pop1_0() {
        hdr.mpls[0].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop1") action _decap_mpls_inner_ipv6_pop1_0() {
        hdr.mpls[0].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop1") action _decap_mpls_inner_ethernet_ipv4_pop1_0() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop1") action _decap_mpls_inner_ethernet_ipv6_pop1_0() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop1") action _decap_mpls_inner_ethernet_non_ip_pop1_0() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".decap_mpls_inner_ipv4_pop2") action _decap_mpls_inner_ipv4_pop2_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop2") action _decap_mpls_inner_ipv6_pop2_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop2") action _decap_mpls_inner_ethernet_ipv4_pop2_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop2") action _decap_mpls_inner_ethernet_ipv6_pop2_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop2") action _decap_mpls_inner_ethernet_non_ip_pop2_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".decap_mpls_inner_ipv4_pop3") action _decap_mpls_inner_ipv4_pop3_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop3") action _decap_mpls_inner_ipv6_pop3_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop3") action _decap_mpls_inner_ethernet_ipv4_pop3_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop3") action _decap_mpls_inner_ethernet_ipv6_pop3_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop3") action _decap_mpls_inner_ethernet_non_ip_pop3_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".tunnel_decap_process_inner") table _tunnel_decap_process_inner {
        actions = {
            _decap_inner_udp_0();
            _decap_inner_tcp_0();
            _decap_inner_icmp_0();
            _decap_inner_unknown_0();
            @defaultonly NoAction_7();
        }
        key = {
            hdr.inner_tcp.isValid() : exact @name("inner_tcp.$valid$");
            hdr.inner_udp.isValid() : exact @name("inner_udp.$valid$");
            hdr.inner_icmp.isValid(): exact @name("inner_icmp.$valid$");
        }
        size = 1024;
        default_action = NoAction_7();
    }
    @name(".tunnel_decap_process_outer") table _tunnel_decap_process_outer {
        actions = {
            _decap_vxlan_inner_ipv4_0();
            _decap_vxlan_inner_ipv6_0();
            _decap_vxlan_inner_non_ip_0();
            _decap_genv_inner_ipv4_0();
            _decap_genv_inner_ipv6_0();
            _decap_genv_inner_non_ip_0();
            _decap_nvgre_inner_ipv4_0();
            _decap_nvgre_inner_ipv6_0();
            _decap_nvgre_inner_non_ip_0();
            _decap_gre_inner_ipv4_0();
            _decap_gre_inner_ipv6_0();
            _decap_gre_inner_non_ip_0();
            _decap_ip_inner_ipv4_0();
            _decap_ip_inner_ipv6_0();
            _decap_mpls_inner_ipv4_pop1_0();
            _decap_mpls_inner_ipv6_pop1_0();
            _decap_mpls_inner_ethernet_ipv4_pop1_0();
            _decap_mpls_inner_ethernet_ipv6_pop1_0();
            _decap_mpls_inner_ethernet_non_ip_pop1_0();
            _decap_mpls_inner_ipv4_pop2_0();
            _decap_mpls_inner_ipv6_pop2_0();
            _decap_mpls_inner_ethernet_ipv4_pop2_0();
            _decap_mpls_inner_ethernet_ipv6_pop2_0();
            _decap_mpls_inner_ethernet_non_ip_pop2_0();
            _decap_mpls_inner_ipv4_pop3_0();
            _decap_mpls_inner_ipv6_pop3_0();
            _decap_mpls_inner_ethernet_ipv4_pop3_0();
            _decap_mpls_inner_ethernet_ipv6_pop3_0();
            _decap_mpls_inner_ethernet_non_ip_pop3_0();
            @defaultonly NoAction_8();
        }
        key = {
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type");
            hdr.inner_ipv4.isValid()                : exact @name("inner_ipv4.$valid$");
            hdr.inner_ipv6.isValid()                : exact @name("inner_ipv6.$valid$");
        }
        size = 1024;
        default_action = NoAction_8();
    }
    @name(".nop") action _nop_2() {
    }
    @name(".nop") action _nop_3() {
    }
    @name(".set_l2_rewrite") action _set_l2_rewrite_0() {
        meta.egress_metadata.routed = 1w0;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.egress_metadata.outer_bd = meta.ingress_metadata.bd;
    }
    @name(".set_l2_rewrite_with_tunnel") action _set_l2_rewrite_with_tunnel_0(@name("tunnel_index") bit<14> tunnel_index_11, @name("tunnel_type") bit<5> tunnel_type_9) {
        meta.egress_metadata.routed = 1w0;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.egress_metadata.outer_bd = meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index_11;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type_9;
    }
    @name(".set_l3_rewrite") action _set_l3_rewrite_0(@name("bd") bit<16> bd_20, @name("mtu_index") bit<8> mtu_index_1, @name("dmac") bit<48> dmac) {
        meta.egress_metadata.routed = 1w1;
        meta.egress_metadata.mac_da = dmac;
        meta.egress_metadata.bd = bd_20;
        meta.egress_metadata.outer_bd = bd_20;
        meta.l3_metadata.mtu_index = mtu_index_1;
    }
    @name(".set_l3_rewrite_with_tunnel") action _set_l3_rewrite_with_tunnel_0(@name("bd") bit<16> bd_21, @name("dmac") bit<48> dmac_0, @name("tunnel_index") bit<14> tunnel_index_12, @name("tunnel_type") bit<5> tunnel_type_10) {
        meta.egress_metadata.routed = 1w1;
        meta.egress_metadata.mac_da = dmac_0;
        meta.egress_metadata.bd = bd_21;
        meta.egress_metadata.outer_bd = bd_21;
        meta.tunnel_metadata.tunnel_index = tunnel_index_12;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type_10;
    }
    @name(".set_mpls_swap_push_rewrite_l2") action _set_mpls_swap_push_rewrite_l2_0(@name("label") bit<20> label_2, @name("tunnel_index") bit<14> tunnel_index_13, @name("header_count") bit<4> header_count_7) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        hdr.mpls[0].label = label_2;
        meta.tunnel_metadata.tunnel_index = tunnel_index_13;
        meta.tunnel_metadata.egress_header_count = header_count_7;
        meta.tunnel_metadata.egress_tunnel_type = 5w13;
    }
    @name(".set_mpls_push_rewrite_l2") action _set_mpls_push_rewrite_l2_0(@name("tunnel_index") bit<14> tunnel_index_14, @name("header_count") bit<4> header_count_8) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index_14;
        meta.tunnel_metadata.egress_header_count = header_count_8;
        meta.tunnel_metadata.egress_tunnel_type = 5w13;
    }
    @name(".set_mpls_swap_push_rewrite_l3") action _set_mpls_swap_push_rewrite_l3_0(@name("bd") bit<16> bd_22, @name("dmac") bit<48> dmac_6, @name("label") bit<20> label_3, @name("tunnel_index") bit<14> tunnel_index_15, @name("header_count") bit<4> header_count_9) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = bd_22;
        hdr.mpls[0].label = label_3;
        meta.egress_metadata.mac_da = dmac_6;
        meta.tunnel_metadata.tunnel_index = tunnel_index_15;
        meta.tunnel_metadata.egress_header_count = header_count_9;
        meta.tunnel_metadata.egress_tunnel_type = 5w14;
    }
    @name(".set_mpls_push_rewrite_l3") action _set_mpls_push_rewrite_l3_0(@name("bd") bit<16> bd_23, @name("dmac") bit<48> dmac_7, @name("tunnel_index") bit<14> tunnel_index_16, @name("header_count") bit<4> header_count_10) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = bd_23;
        meta.egress_metadata.mac_da = dmac_7;
        meta.tunnel_metadata.tunnel_index = tunnel_index_16;
        meta.tunnel_metadata.egress_header_count = header_count_10;
        meta.tunnel_metadata.egress_tunnel_type = 5w14;
    }
    @name(".rewrite_ipv4_multicast") action _rewrite_ipv4_multicast_0() {
        hdr.ethernet.dstAddr[22:0] = hdr.ipv4.dstAddr[22:0];
    }
    @name(".rewrite_ipv6_multicast") action _rewrite_ipv6_multicast_0() {
    }
    @name(".rewrite") table _rewrite {
        actions = {
            _nop_2();
            _set_l2_rewrite_0();
            _set_l2_rewrite_with_tunnel_0();
            _set_l3_rewrite_0();
            _set_l3_rewrite_with_tunnel_0();
            _set_mpls_swap_push_rewrite_l2_0();
            _set_mpls_push_rewrite_l2_0();
            _set_mpls_swap_push_rewrite_l3_0();
            _set_mpls_push_rewrite_l3_0();
            @defaultonly NoAction_9();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("l3_metadata.nexthop_index");
        }
        size = 1024;
        default_action = NoAction_9();
    }
    @name(".rewrite_multicast") table _rewrite_multicast {
        actions = {
            _nop_3();
            _rewrite_ipv4_multicast_0();
            _rewrite_ipv6_multicast_0();
            @defaultonly NoAction_10();
        }
        key = {
            hdr.ipv4.isValid()       : exact @name("ipv4.$valid$");
            hdr.ipv6.isValid()       : exact @name("ipv6.$valid$");
            hdr.ipv4.dstAddr[31:28]  : ternary @name("ipv4.dstAddr");
            hdr.ipv6.dstAddr[127:120]: ternary @name("ipv6.dstAddr");
        }
        default_action = NoAction_10();
    }
    @name(".nop") action _nop_4() {
    }
    @name(".set_egress_bd_properties") action _set_egress_bd_properties_0(@name("smac_idx") bit<9> smac_idx_5) {
        meta.egress_metadata.smac_idx = smac_idx_5;
    }
    @name(".egress_bd_map") table _egress_bd_map {
        actions = {
            _nop_4();
            _set_egress_bd_properties_0();
            @defaultonly NoAction_11();
        }
        key = {
            meta.egress_metadata.bd: exact @name("egress_metadata.bd");
        }
        size = 1024;
        default_action = NoAction_11();
    }
    @name(".nop") action _nop_5() {
    }
    @name(".ipv4_unicast_rewrite") action _ipv4_unicast_rewrite_0() {
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".ipv4_multicast_rewrite") action _ipv4_multicast_rewrite_0() {
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | 48w0x1005e000000;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".ipv6_unicast_rewrite") action _ipv6_unicast_rewrite_0() {
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit + 8w255;
    }
    @name(".ipv6_multicast_rewrite") action _ipv6_multicast_rewrite_0() {
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | 48w0x333300000000;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit + 8w255;
    }
    @name(".mpls_rewrite") action _mpls_rewrite_0() {
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.mpls[0].ttl = hdr.mpls[0].ttl + 8w255;
    }
    @name(".rewrite_smac") action _rewrite_smac_0(@name("smac") bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name(".l3_rewrite") table _l3_rewrite {
        actions = {
            _nop_5();
            _ipv4_unicast_rewrite_0();
            _ipv4_multicast_rewrite_0();
            _ipv6_unicast_rewrite_0();
            _ipv6_multicast_rewrite_0();
            _mpls_rewrite_0();
            @defaultonly NoAction_12();
        }
        key = {
            hdr.ipv4.isValid()       : exact @name("ipv4.$valid$");
            hdr.ipv6.isValid()       : exact @name("ipv6.$valid$");
            hdr.mpls[0].isValid()    : exact @name("mpls[0].$valid$");
            hdr.ipv4.dstAddr[31:28]  : ternary @name("ipv4.dstAddr");
            hdr.ipv6.dstAddr[127:120]: ternary @name("ipv6.dstAddr");
        }
        default_action = NoAction_12();
    }
    @name(".smac_rewrite") table _smac_rewrite {
        actions = {
            _rewrite_smac_0();
            @defaultonly NoAction_13();
        }
        key = {
            meta.egress_metadata.smac_idx: exact @name("egress_metadata.smac_idx");
        }
        size = 512;
        default_action = NoAction_13();
    }
    @name(".mtu_miss") action _mtu_miss_0() {
        meta.l3_metadata.l3_mtu_check = 16w0xffff;
    }
    @name(".ipv4_mtu_check") action _ipv4_mtu_check_0(@name("l3_mtu") bit<16> l3_mtu) {
        meta.l3_metadata.l3_mtu_check = l3_mtu |-| hdr.ipv4.totalLen;
    }
    @name(".ipv6_mtu_check") action _ipv6_mtu_check_0(@name("l3_mtu") bit<16> l3_mtu_3) {
        meta.l3_metadata.l3_mtu_check = l3_mtu_3 |-| hdr.ipv6.payloadLen;
    }
    @name(".mtu") table _mtu {
        actions = {
            _mtu_miss_0();
            _ipv4_mtu_check_0();
            _ipv6_mtu_check_0();
            @defaultonly NoAction_14();
        }
        key = {
            meta.l3_metadata.mtu_index: exact @name("l3_metadata.mtu_index");
            hdr.ipv4.isValid()        : exact @name("ipv4.$valid$");
            hdr.ipv6.isValid()        : exact @name("ipv6.$valid$");
        }
        size = 1024;
        default_action = NoAction_14();
    }
    @name(".int_set_header_0_bos") action _int_set_header_0_bos_0() {
        hdr.int_switch_id_header.bos = 1w1;
    }
    @name(".int_set_header_1_bos") action _int_set_header_1_bos_0() {
        hdr.int_ingress_port_id_header.bos = 1w1;
    }
    @name(".int_set_header_2_bos") action _int_set_header_2_bos_0() {
        hdr.int_hop_latency_header.bos = 1w1;
    }
    @name(".int_set_header_3_bos") action _int_set_header_3_bos_0() {
        hdr.int_q_occupancy_header.bos = 1w1;
    }
    @name(".int_set_header_4_bos") action _int_set_header_4_bos_0() {
        hdr.int_ingress_tstamp_header.bos = 1w1;
    }
    @name(".int_set_header_5_bos") action _int_set_header_5_bos_0() {
        hdr.int_egress_port_id_header.bos = 1w1;
    }
    @name(".int_set_header_6_bos") action _int_set_header_6_bos_0() {
        hdr.int_q_congestion_header.bos = 1w1;
    }
    @name(".int_set_header_7_bos") action _int_set_header_7_bos_0() {
        hdr.int_egress_port_tx_utilization_header.bos = 1w1;
    }
    @name(".nop") action _nop_6() {
    }
    @name(".nop") action _nop_7() {
    }
    @name(".nop") action _nop_46() {
    }
    @name(".nop") action _nop_47() {
    }
    @name(".int_transit") action _int_transit_0(@name("switch_id") bit<32> switch_id_2) {
        meta.int_metadata.insert_cnt = hdr.int_header.max_hop_cnt - hdr.int_header.total_hop_cnt;
        meta.int_metadata.switch_id = switch_id_2;
        meta.int_metadata.insert_byte_cnt = meta.int_metadata.instruction_cnt << 2;
        meta.int_metadata.gpe_int_hdr_len8 = (bit<8>)hdr.int_header.ins_cnt;
    }
    @name(".int_src") action _int_src_0(@name("switch_id") bit<32> switch_id_3, @name("hop_cnt") bit<8> hop_cnt, @name("ins_cnt") bit<5> ins_cnt_1, @name("ins_mask0003") bit<4> ins_mask0003, @name("ins_mask0407") bit<4> ins_mask0407, @name("ins_byte_cnt") bit<16> ins_byte_cnt, @name("total_words") bit<8> total_words) {
        meta.int_metadata.insert_cnt = hop_cnt;
        meta.int_metadata.switch_id = switch_id_3;
        meta.int_metadata.insert_byte_cnt = ins_byte_cnt;
        meta.int_metadata.gpe_int_hdr_len8 = total_words;
        hdr.int_header.setValid();
        hdr.int_header.ver = 2w0;
        hdr.int_header.rep = 2w0;
        hdr.int_header.c = 1w0;
        hdr.int_header.e = 1w0;
        hdr.int_header.rsvd1 = 5w0;
        hdr.int_header.ins_cnt = ins_cnt_1;
        hdr.int_header.max_hop_cnt = hop_cnt;
        hdr.int_header.total_hop_cnt = 8w0;
        hdr.int_header.instruction_mask_0003 = ins_mask0003;
        hdr.int_header.instruction_mask_0407 = ins_mask0407;
        hdr.int_header.instruction_mask_0811 = 4w0;
        hdr.int_header.instruction_mask_1215 = 4w0;
        hdr.int_header.rsvd2 = 16w0;
    }
    @name(".int_reset") action _int_reset_0() {
        meta.int_metadata.switch_id = 32w0;
        meta.int_metadata.insert_byte_cnt = 16w0;
        meta.int_metadata.insert_cnt = 8w0;
        meta.int_metadata.gpe_int_hdr_len8 = 8w0;
        meta.int_metadata.gpe_int_hdr_len = 16w0;
        meta.int_metadata.instruction_cnt = 16w0;
    }
    @name(".int_set_header_0003_i0") action _int_set_header_0003_i0_0() {
    }
    @name(".int_set_header_0003_i1") action _int_set_header_0003_i1_0() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy1 = 7w0;
        hdr.int_q_occupancy_header.q_occupancy0 = (bit<24>)standard_metadata.enq_qdepth;
    }
    @name(".int_set_header_0003_i2") action _int_set_header_0003_i2_0() {
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)standard_metadata.deq_timedelta;
    }
    @name(".int_set_header_0003_i3") action _int_set_header_0003_i3_0() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy1 = 7w0;
        hdr.int_q_occupancy_header.q_occupancy0 = (bit<24>)standard_metadata.enq_qdepth;
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)standard_metadata.deq_timedelta;
    }
    @name(".int_set_header_0003_i4") action _int_set_header_0003_i4_0() {
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id_1 = 15w0;
        hdr.int_ingress_port_id_header.ingress_port_id_0 = meta.ingress_metadata.ifindex;
    }
    @name(".int_set_header_0003_i5") action _int_set_header_0003_i5_0() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy1 = 7w0;
        hdr.int_q_occupancy_header.q_occupancy0 = (bit<24>)standard_metadata.enq_qdepth;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id_1 = 15w0;
        hdr.int_ingress_port_id_header.ingress_port_id_0 = meta.ingress_metadata.ifindex;
    }
    @name(".int_set_header_0003_i6") action _int_set_header_0003_i6_0() {
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)standard_metadata.deq_timedelta;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id_1 = 15w0;
        hdr.int_ingress_port_id_header.ingress_port_id_0 = meta.ingress_metadata.ifindex;
    }
    @name(".int_set_header_0003_i7") action _int_set_header_0003_i7_0() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy1 = 7w0;
        hdr.int_q_occupancy_header.q_occupancy0 = (bit<24>)standard_metadata.enq_qdepth;
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)standard_metadata.deq_timedelta;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id_1 = 15w0;
        hdr.int_ingress_port_id_header.ingress_port_id_0 = meta.ingress_metadata.ifindex;
    }
    @name(".int_set_header_0003_i8") action _int_set_header_0003_i8_0() {
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i9") action _int_set_header_0003_i9_0() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy1 = 7w0;
        hdr.int_q_occupancy_header.q_occupancy0 = (bit<24>)standard_metadata.enq_qdepth;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i10") action _int_set_header_0003_i10_0() {
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)standard_metadata.deq_timedelta;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i11") action _int_set_header_0003_i11_0() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy1 = 7w0;
        hdr.int_q_occupancy_header.q_occupancy0 = (bit<24>)standard_metadata.enq_qdepth;
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)standard_metadata.deq_timedelta;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i12") action _int_set_header_0003_i12_0() {
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id_1 = 15w0;
        hdr.int_ingress_port_id_header.ingress_port_id_0 = meta.ingress_metadata.ifindex;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i13") action _int_set_header_0003_i13_0() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy1 = 7w0;
        hdr.int_q_occupancy_header.q_occupancy0 = (bit<24>)standard_metadata.enq_qdepth;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id_1 = 15w0;
        hdr.int_ingress_port_id_header.ingress_port_id_0 = meta.ingress_metadata.ifindex;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i14") action _int_set_header_0003_i14_0() {
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)standard_metadata.deq_timedelta;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id_1 = 15w0;
        hdr.int_ingress_port_id_header.ingress_port_id_0 = meta.ingress_metadata.ifindex;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i15") action _int_set_header_0003_i15_0() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy1 = 7w0;
        hdr.int_q_occupancy_header.q_occupancy0 = (bit<24>)standard_metadata.enq_qdepth;
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)standard_metadata.deq_timedelta;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id_1 = 15w0;
        hdr.int_ingress_port_id_header.ingress_port_id_0 = meta.ingress_metadata.ifindex;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0407_i0") action _int_set_header_0407_i0_0() {
    }
    @name(".int_set_header_0407_i1") action _int_set_header_0407_i1_0() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
    }
    @name(".int_set_header_0407_i2") action _int_set_header_0407_i2_0() {
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
    }
    @name(".int_set_header_0407_i3") action _int_set_header_0407_i3_0() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
    }
    @name(".int_set_header_0407_i4") action _int_set_header_0407_i4_0() {
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
    }
    @name(".int_set_header_0407_i5") action _int_set_header_0407_i5_0() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
    }
    @name(".int_set_header_0407_i6") action _int_set_header_0407_i6_0() {
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
    }
    @name(".int_set_header_0407_i7") action _int_set_header_0407_i7_0() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
    }
    @name(".int_set_header_0407_i8") action _int_set_header_0407_i8_0() {
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i9") action _int_set_header_0407_i9_0() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i10") action _int_set_header_0407_i10_0() {
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i11") action _int_set_header_0407_i11_0() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i12") action _int_set_header_0407_i12_0() {
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i13") action _int_set_header_0407_i13_0() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i14") action _int_set_header_0407_i14_0() {
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i15") action _int_set_header_0407_i15_0() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_e_bit") action _int_set_e_bit_0() {
        hdr.int_header.e = 1w1;
    }
    @name(".int_update_total_hop_cnt") action _int_update_total_hop_cnt_0() {
        hdr.int_header.total_hop_cnt = hdr.int_header.total_hop_cnt + 8w1;
    }
    @name(".int_bos") table _int_bos {
        actions = {
            _int_set_header_0_bos_0();
            _int_set_header_1_bos_0();
            _int_set_header_2_bos_0();
            _int_set_header_3_bos_0();
            _int_set_header_4_bos_0();
            _int_set_header_5_bos_0();
            _int_set_header_6_bos_0();
            _int_set_header_7_bos_0();
            _nop_6();
            @defaultonly NoAction_15();
        }
        key = {
            hdr.int_header.total_hop_cnt        : ternary @name("int_header.total_hop_cnt");
            hdr.int_header.instruction_mask_0003: ternary @name("int_header.instruction_mask_0003");
            hdr.int_header.instruction_mask_0407: ternary @name("int_header.instruction_mask_0407");
            hdr.int_header.instruction_mask_0811: ternary @name("int_header.instruction_mask_0811");
            hdr.int_header.instruction_mask_1215: ternary @name("int_header.instruction_mask_1215");
        }
        size = 17;
        default_action = NoAction_15();
    }
    @name(".int_insert") table _int_insert {
        actions = {
            _int_transit_0();
            _int_src_0();
            _int_reset_0();
            @defaultonly NoAction_16();
        }
        key = {
            meta.int_metadata_i2e.source: ternary @name("int_metadata_i2e.source");
            meta.int_metadata_i2e.sink  : ternary @name("int_metadata_i2e.sink");
            hdr.int_header.isValid()    : exact @name("int_header.$valid$");
        }
        size = 3;
        default_action = NoAction_16();
    }
    @name(".int_inst_0003") table _int_inst {
        actions = {
            _int_set_header_0003_i0_0();
            _int_set_header_0003_i1_0();
            _int_set_header_0003_i2_0();
            _int_set_header_0003_i3_0();
            _int_set_header_0003_i4_0();
            _int_set_header_0003_i5_0();
            _int_set_header_0003_i6_0();
            _int_set_header_0003_i7_0();
            _int_set_header_0003_i8_0();
            _int_set_header_0003_i9_0();
            _int_set_header_0003_i10_0();
            _int_set_header_0003_i11_0();
            _int_set_header_0003_i12_0();
            _int_set_header_0003_i13_0();
            _int_set_header_0003_i14_0();
            _int_set_header_0003_i15_0();
            @defaultonly NoAction_17();
        }
        key = {
            hdr.int_header.instruction_mask_0003: exact @name("int_header.instruction_mask_0003");
        }
        size = 17;
        default_action = NoAction_17();
    }
    @name(".int_inst_0407") table _int_inst_0 {
        actions = {
            _int_set_header_0407_i0_0();
            _int_set_header_0407_i1_0();
            _int_set_header_0407_i2_0();
            _int_set_header_0407_i3_0();
            _int_set_header_0407_i4_0();
            _int_set_header_0407_i5_0();
            _int_set_header_0407_i6_0();
            _int_set_header_0407_i7_0();
            _int_set_header_0407_i8_0();
            _int_set_header_0407_i9_0();
            _int_set_header_0407_i10_0();
            _int_set_header_0407_i11_0();
            _int_set_header_0407_i12_0();
            _int_set_header_0407_i13_0();
            _int_set_header_0407_i14_0();
            _int_set_header_0407_i15_0();
            _nop_7();
            @defaultonly NoAction_18();
        }
        key = {
            hdr.int_header.instruction_mask_0407: exact @name("int_header.instruction_mask_0407");
        }
        size = 17;
        default_action = NoAction_18();
    }
    @name(".int_inst_0811") table _int_inst_1 {
        actions = {
            _nop_46();
            @defaultonly NoAction_19();
        }
        key = {
            hdr.int_header.instruction_mask_0811: exact @name("int_header.instruction_mask_0811");
        }
        size = 16;
        default_action = NoAction_19();
    }
    @name(".int_inst_1215") table _int_inst_2 {
        actions = {
            _nop_47();
            @defaultonly NoAction_20();
        }
        key = {
            hdr.int_header.instruction_mask_1215: exact @name("int_header.instruction_mask_1215");
        }
        size = 17;
        default_action = NoAction_20();
    }
    @name(".int_meta_header_update") table _int_meta_header_update {
        actions = {
            _int_set_e_bit_0();
            _int_update_total_hop_cnt_0();
            @defaultonly NoAction_21();
        }
        key = {
            meta.int_metadata.insert_cnt: ternary @name("int_metadata.insert_cnt");
        }
        size = 2;
        default_action = NoAction_21();
    }
    @min_width(32) @name(".egress_bd_stats") direct_counter(CounterType.packets_and_bytes) _egress_bd_stats;
    @name(".nop") action _nop_48() {
        _egress_bd_stats.count();
    }
    @name(".egress_bd_stats") table _egress_bd_stats_0 {
        actions = {
            _nop_48();
            @defaultonly NoAction_22();
        }
        key = {
            meta.egress_metadata.bd      : exact @name("egress_metadata.bd");
            meta.l2_metadata.lkp_pkt_type: exact @name("l2_metadata.lkp_pkt_type");
        }
        size = 1024;
        counters = _egress_bd_stats;
        default_action = NoAction_22();
    }
    @name(".nop") action _nop_49() {
    }
    @name(".nop") action _nop_50() {
    }
    @name(".nop") action _nop_51() {
    }
    @name(".nop") action _nop_52() {
    }
    @name(".nop") action _nop_53() {
    }
    @name(".nop") action _nop_54() {
    }
    @name(".nop") action _nop_55() {
    }
    @name(".set_egress_tunnel_vni") action _set_egress_tunnel_vni_0(@name("vnid") bit<24> vnid_1) {
        meta.tunnel_metadata.vnid = vnid_1;
    }
    @name(".rewrite_tunnel_dmac") action _rewrite_tunnel_dmac_0(@name("dmac") bit<48> dmac_8) {
        hdr.ethernet.dstAddr = dmac_8;
    }
    @name(".rewrite_tunnel_ipv4_dst") action _rewrite_tunnel_ipv4_dst_0(@name("ip") bit<32> ip) {
        hdr.ipv4.dstAddr = ip;
    }
    @name(".rewrite_tunnel_ipv6_dst") action _rewrite_tunnel_ipv6_dst_0(@name("ip") bit<128> ip_4) {
        hdr.ipv6.dstAddr = ip_4;
    }
    @name(".inner_ipv4_udp_rewrite") action _inner_ipv4_udp_rewrite_0() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_udp = hdr.udp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.udp.setInvalid();
        hdr.ipv4.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w4;
    }
    @name(".inner_ipv4_tcp_rewrite") action _inner_ipv4_tcp_rewrite_0() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_tcp = hdr.tcp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.tcp.setInvalid();
        hdr.ipv4.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w4;
    }
    @name(".inner_ipv4_icmp_rewrite") action _inner_ipv4_icmp_rewrite_0() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_icmp = hdr.icmp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.icmp.setInvalid();
        hdr.ipv4.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w4;
    }
    @name(".inner_ipv4_unknown_rewrite") action _inner_ipv4_unknown_rewrite_0() {
        hdr.inner_ipv4 = hdr.ipv4;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.ipv4.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w4;
    }
    @name(".inner_ipv6_udp_rewrite") action _inner_ipv6_udp_rewrite_0() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_udp = hdr.udp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.ipv6.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w41;
    }
    @name(".inner_ipv6_tcp_rewrite") action _inner_ipv6_tcp_rewrite_0() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_tcp = hdr.tcp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.tcp.setInvalid();
        hdr.ipv6.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w41;
    }
    @name(".inner_ipv6_icmp_rewrite") action _inner_ipv6_icmp_rewrite_0() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_icmp = hdr.icmp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.icmp.setInvalid();
        hdr.ipv6.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w41;
    }
    @name(".inner_ipv6_unknown_rewrite") action _inner_ipv6_unknown_rewrite_0() {
        hdr.inner_ipv6 = hdr.ipv6;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.ipv6.setInvalid();
        meta.tunnel_metadata.inner_ip_proto = 8w41;
    }
    @name(".inner_non_ip_rewrite") action _inner_non_ip_rewrite_0() {
        meta.egress_metadata.payload_length = (bit<16>)standard_metadata.packet_length + 16w65522;
    }
    @name(".ipv4_vxlan_rewrite") action _ipv4_vxlan_rewrite_0() {
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
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = 8w17;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w50;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv4_genv_rewrite") action _ipv4_genv_rewrite_0() {
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
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = 8w17;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w50;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv4_nvgre_rewrite") action _ipv4_nvgre_rewrite_0() {
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
        hdr.nvgre.flow_id = meta.hash_metadata.entropy_hash[7:0];
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = 8w47;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w42;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv4_gre_rewrite") action _ipv4_gre_rewrite_0() {
        hdr.gre.setValid();
        hdr.gre.proto = hdr.ethernet.etherType;
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = 8w47;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w24;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv4_ip_rewrite") action _ipv4_ip_rewrite_0() {
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = meta.tunnel_metadata.inner_ip_proto;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w20;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv4_erspan_t3_rewrite") action _ipv4_erspan_t3_rewrite_0() {
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
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = 8w47;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w50;
    }
    @name(".ipv6_gre_rewrite") action _ipv6_gre_rewrite_0() {
        hdr.gre.setValid();
        hdr.gre.proto = hdr.ethernet.etherType;
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = 8w47;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w4;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_ip_rewrite") action _ipv6_ip_rewrite_0() {
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = meta.tunnel_metadata.inner_ip_proto;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_nvgre_rewrite") action _ipv6_nvgre_rewrite_0() {
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
        hdr.nvgre.flow_id = meta.hash_metadata.entropy_hash[7:0];
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = 8w47;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w22;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_vxlan_rewrite") action _ipv6_vxlan_rewrite_0() {
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
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = 8w17;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w30;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_genv_rewrite") action _ipv6_genv_rewrite_0() {
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
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = 8w17;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w30;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_erspan_t3_rewrite") action _ipv6_erspan_t3_rewrite_0() {
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
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = 8w47;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w26;
    }
    @name(".mpls_ethernet_push1_rewrite") action _mpls_ethernet_push1_rewrite_0() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.mpls.push_front(1);
        hdr.mpls[0].setValid();
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push1_rewrite") action _mpls_ip_push1_rewrite_0() {
        hdr.mpls.push_front(1);
        hdr.mpls[0].setValid();
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ethernet_push2_rewrite") action _mpls_ethernet_push2_rewrite_0() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.mpls.push_front(2);
        hdr.mpls[0].setValid();
        hdr.mpls[1].setValid();
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push2_rewrite") action _mpls_ip_push2_rewrite_0() {
        hdr.mpls.push_front(2);
        hdr.mpls[0].setValid();
        hdr.mpls[1].setValid();
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ethernet_push3_rewrite") action _mpls_ethernet_push3_rewrite_0() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.mpls.push_front(3);
        hdr.mpls[0].setValid();
        hdr.mpls[1].setValid();
        hdr.mpls[2].setValid();
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push3_rewrite") action _mpls_ip_push3_rewrite_0() {
        hdr.mpls.push_front(3);
        hdr.mpls[0].setValid();
        hdr.mpls[1].setValid();
        hdr.mpls[2].setValid();
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".fabric_rewrite") action _fabric_rewrite_0(@name("tunnel_index") bit<14> tunnel_index_17) {
        meta.tunnel_metadata.tunnel_index = tunnel_index_17;
    }
    @name(".tunnel_mtu_check") action _tunnel_mtu_check_0(@name("l3_mtu") bit<16> l3_mtu_4) {
        meta.l3_metadata.l3_mtu_check = l3_mtu_4 |-| meta.egress_metadata.payload_length;
    }
    @name(".tunnel_mtu_miss") action _tunnel_mtu_miss_0() {
        meta.l3_metadata.l3_mtu_check = 16w0xffff;
    }
    @name(".set_tunnel_rewrite_details") action _set_tunnel_rewrite_details_0(@name("outer_bd") bit<16> outer_bd_1, @name("smac_idx") bit<9> smac_idx_6, @name("dmac_idx") bit<14> dmac_idx, @name("sip_index") bit<9> sip_index, @name("dip_index") bit<14> dip_index) {
        meta.egress_metadata.outer_bd = outer_bd_1;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx_6;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx;
        meta.tunnel_metadata.tunnel_src_index = sip_index;
        meta.tunnel_metadata.tunnel_dst_index = dip_index;
    }
    @name(".set_mpls_rewrite_push1") action _set_mpls_rewrite_push1_0(@name("label1") bit<20> label1, @name("exp1") bit<3> exp1, @name("ttl1") bit<8> ttl1, @name("smac_idx") bit<9> smac_idx_7, @name("dmac_idx") bit<14> dmac_idx_4) {
        hdr.mpls[0].label = label1;
        hdr.mpls[0].exp = exp1;
        hdr.mpls[0].bos = 1w0x1;
        hdr.mpls[0].ttl = ttl1;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx_7;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx_4;
    }
    @name(".set_mpls_rewrite_push2") action _set_mpls_rewrite_push2_0(@name("label1") bit<20> label1_3, @name("exp1") bit<3> exp1_3, @name("ttl1") bit<8> ttl1_3, @name("label2") bit<20> label2, @name("exp2") bit<3> exp2, @name("ttl2") bit<8> ttl2, @name("smac_idx") bit<9> smac_idx_8, @name("dmac_idx") bit<14> dmac_idx_5) {
        hdr.mpls[0].label = label1_3;
        hdr.mpls[0].exp = exp1_3;
        hdr.mpls[0].ttl = ttl1_3;
        hdr.mpls[0].bos = 1w0x0;
        hdr.mpls[1].label = label2;
        hdr.mpls[1].exp = exp2;
        hdr.mpls[1].ttl = ttl2;
        hdr.mpls[1].bos = 1w0x1;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx_8;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx_5;
    }
    @name(".set_mpls_rewrite_push3") action _set_mpls_rewrite_push3_0(@name("label1") bit<20> label1_4, @name("exp1") bit<3> exp1_4, @name("ttl1") bit<8> ttl1_4, @name("label2") bit<20> label2_2, @name("exp2") bit<3> exp2_2, @name("ttl2") bit<8> ttl2_2, @name("label3") bit<20> label3, @name("exp3") bit<3> exp3, @name("ttl3") bit<8> ttl3, @name("smac_idx") bit<9> smac_idx_9, @name("dmac_idx") bit<14> dmac_idx_6) {
        hdr.mpls[0].label = label1_4;
        hdr.mpls[0].exp = exp1_4;
        hdr.mpls[0].ttl = ttl1_4;
        hdr.mpls[0].bos = 1w0x0;
        hdr.mpls[1].label = label2_2;
        hdr.mpls[1].exp = exp2_2;
        hdr.mpls[1].ttl = ttl2_2;
        hdr.mpls[1].bos = 1w0x0;
        hdr.mpls[2].label = label3;
        hdr.mpls[2].exp = exp3;
        hdr.mpls[2].ttl = ttl3;
        hdr.mpls[2].bos = 1w0x1;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx_9;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx_6;
    }
    @name(".cpu_rx_rewrite") action _cpu_rx_rewrite_0() {
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
    @name(".fabric_unicast_rewrite") action _fabric_unicast_rewrite_0() {
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
    @name(".fabric_multicast_rewrite") action _fabric_multicast_rewrite_0(@name("fabric_mgid") bit<16> fabric_mgid) {
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
    @name(".rewrite_tunnel_smac") action _rewrite_tunnel_smac_0(@name("smac") bit<48> smac_0) {
        hdr.ethernet.srcAddr = smac_0;
    }
    @name(".rewrite_tunnel_ipv4_src") action _rewrite_tunnel_ipv4_src_0(@name("ip") bit<32> ip_5) {
        hdr.ipv4.srcAddr = ip_5;
    }
    @name(".rewrite_tunnel_ipv6_src") action _rewrite_tunnel_ipv6_src_0(@name("ip") bit<128> ip_6) {
        hdr.ipv6.srcAddr = ip_6;
    }
    @name(".egress_vni") table _egress_vni {
        actions = {
            _nop_49();
            _set_egress_tunnel_vni_0();
            @defaultonly NoAction_23();
        }
        key = {
            meta.egress_metadata.bd                : exact @name("egress_metadata.bd");
            meta.tunnel_metadata.egress_tunnel_type: exact @name("tunnel_metadata.egress_tunnel_type");
        }
        size = 1024;
        default_action = NoAction_23();
    }
    @name(".tunnel_dmac_rewrite") table _tunnel_dmac_rewrite {
        actions = {
            _nop_50();
            _rewrite_tunnel_dmac_0();
            @defaultonly NoAction_24();
        }
        key = {
            meta.tunnel_metadata.tunnel_dmac_index: exact @name("tunnel_metadata.tunnel_dmac_index");
        }
        size = 1024;
        default_action = NoAction_24();
    }
    @name(".tunnel_dst_rewrite") table _tunnel_dst_rewrite {
        actions = {
            _nop_51();
            _rewrite_tunnel_ipv4_dst_0();
            _rewrite_tunnel_ipv6_dst_0();
            @defaultonly NoAction_25();
        }
        key = {
            meta.tunnel_metadata.tunnel_dst_index: exact @name("tunnel_metadata.tunnel_dst_index");
        }
        size = 1024;
        default_action = NoAction_25();
    }
    @name(".tunnel_encap_process_inner") table _tunnel_encap_process_inner {
        actions = {
            _inner_ipv4_udp_rewrite_0();
            _inner_ipv4_tcp_rewrite_0();
            _inner_ipv4_icmp_rewrite_0();
            _inner_ipv4_unknown_rewrite_0();
            _inner_ipv6_udp_rewrite_0();
            _inner_ipv6_tcp_rewrite_0();
            _inner_ipv6_icmp_rewrite_0();
            _inner_ipv6_unknown_rewrite_0();
            _inner_non_ip_rewrite_0();
            @defaultonly NoAction_26();
        }
        key = {
            hdr.ipv4.isValid(): exact @name("ipv4.$valid$");
            hdr.ipv6.isValid(): exact @name("ipv6.$valid$");
            hdr.tcp.isValid() : exact @name("tcp.$valid$");
            hdr.udp.isValid() : exact @name("udp.$valid$");
            hdr.icmp.isValid(): exact @name("icmp.$valid$");
        }
        size = 1024;
        default_action = NoAction_26();
    }
    @name(".tunnel_encap_process_outer") table _tunnel_encap_process_outer {
        actions = {
            _nop_52();
            _ipv4_vxlan_rewrite_0();
            _ipv4_genv_rewrite_0();
            _ipv4_nvgre_rewrite_0();
            _ipv4_gre_rewrite_0();
            _ipv4_ip_rewrite_0();
            _ipv4_erspan_t3_rewrite_0();
            _ipv6_gre_rewrite_0();
            _ipv6_ip_rewrite_0();
            _ipv6_nvgre_rewrite_0();
            _ipv6_vxlan_rewrite_0();
            _ipv6_genv_rewrite_0();
            _ipv6_erspan_t3_rewrite_0();
            _mpls_ethernet_push1_rewrite_0();
            _mpls_ip_push1_rewrite_0();
            _mpls_ethernet_push2_rewrite_0();
            _mpls_ip_push2_rewrite_0();
            _mpls_ethernet_push3_rewrite_0();
            _mpls_ip_push3_rewrite_0();
            _fabric_rewrite_0();
            @defaultonly NoAction_27();
        }
        key = {
            meta.tunnel_metadata.egress_tunnel_type : exact @name("tunnel_metadata.egress_tunnel_type");
            meta.tunnel_metadata.egress_header_count: exact @name("tunnel_metadata.egress_header_count");
            meta.multicast_metadata.replica         : exact @name("multicast_metadata.replica");
        }
        size = 1024;
        default_action = NoAction_27();
    }
    @name(".tunnel_mtu") table _tunnel_mtu {
        actions = {
            _tunnel_mtu_check_0();
            _tunnel_mtu_miss_0();
            @defaultonly NoAction_28();
        }
        key = {
            meta.tunnel_metadata.tunnel_index: exact @name("tunnel_metadata.tunnel_index");
        }
        size = 1024;
        default_action = NoAction_28();
    }
    @name(".tunnel_rewrite") table _tunnel_rewrite {
        actions = {
            _nop_53();
            _set_tunnel_rewrite_details_0();
            _set_mpls_rewrite_push1_0();
            _set_mpls_rewrite_push2_0();
            _set_mpls_rewrite_push3_0();
            _cpu_rx_rewrite_0();
            _fabric_unicast_rewrite_0();
            _fabric_multicast_rewrite_0();
            @defaultonly NoAction_29();
        }
        key = {
            meta.tunnel_metadata.tunnel_index: exact @name("tunnel_metadata.tunnel_index");
        }
        size = 1024;
        default_action = NoAction_29();
    }
    @name(".tunnel_smac_rewrite") table _tunnel_smac_rewrite {
        actions = {
            _nop_54();
            _rewrite_tunnel_smac_0();
            @defaultonly NoAction_30();
        }
        key = {
            meta.tunnel_metadata.tunnel_smac_index: exact @name("tunnel_metadata.tunnel_smac_index");
        }
        size = 1024;
        default_action = NoAction_30();
    }
    @name(".tunnel_src_rewrite") table _tunnel_src_rewrite {
        actions = {
            _nop_55();
            _rewrite_tunnel_ipv4_src_0();
            _rewrite_tunnel_ipv6_src_0();
            @defaultonly NoAction_31();
        }
        key = {
            meta.tunnel_metadata.tunnel_src_index: exact @name("tunnel_metadata.tunnel_src_index");
        }
        size = 1024;
        default_action = NoAction_31();
    }
    @name(".int_update_vxlan_gpe_ipv4") action _int_update_vxlan_gpe_ipv4_0() {
        hdr.ipv4.totalLen = hdr.ipv4.totalLen + meta.int_metadata.insert_byte_cnt;
        hdr.udp.length_ = hdr.udp.length_ + meta.int_metadata.insert_byte_cnt;
        hdr.vxlan_gpe_int_header.len = hdr.vxlan_gpe_int_header.len + meta.int_metadata.gpe_int_hdr_len8;
    }
    @name(".int_add_update_vxlan_gpe_ipv4") action _int_add_update_vxlan_gpe_ipv4_0() {
        hdr.vxlan_gpe_int_header.setValid();
        hdr.vxlan_gpe_int_header.int_type = 8w0x1;
        hdr.vxlan_gpe_int_header.next_proto = 8w3;
        hdr.vxlan_gpe.next_proto = 8w5;
        hdr.vxlan_gpe_int_header.len = meta.int_metadata.gpe_int_hdr_len8;
        hdr.ipv4.totalLen = hdr.ipv4.totalLen + meta.int_metadata.insert_byte_cnt;
        hdr.udp.length_ = hdr.udp.length_ + meta.int_metadata.insert_byte_cnt;
    }
    @name(".nop") action _nop_56() {
    }
    @name(".int_outer_encap") table _int_outer_encap {
        actions = {
            _int_update_vxlan_gpe_ipv4_0();
            _int_add_update_vxlan_gpe_ipv4_0();
            _nop_56();
            @defaultonly NoAction_32();
        }
        key = {
            hdr.ipv4.isValid()                     : exact @name("ipv4.$valid$");
            hdr.vxlan_gpe.isValid()                : exact @name("vxlan_gpe.$valid$");
            meta.int_metadata_i2e.source           : exact @name("int_metadata_i2e.source");
            meta.tunnel_metadata.egress_tunnel_type: ternary @name("tunnel_metadata.egress_tunnel_type");
        }
        size = 8;
        default_action = NoAction_32();
    }
    @name(".set_egress_packet_vlan_untagged") action _set_egress_packet_vlan_untagged_0() {
    }
    @name(".set_egress_packet_vlan_tagged") action _set_egress_packet_vlan_tagged_0(@name("vlan_id") bit<12> vlan_id) {
        hdr.vlan_tag_[0].setValid();
        hdr.vlan_tag_[0].etherType = hdr.ethernet.etherType;
        hdr.vlan_tag_[0].vid = vlan_id;
        hdr.ethernet.etherType = 16w0x8100;
    }
    @name(".set_egress_packet_vlan_double_tagged") action _set_egress_packet_vlan_double_tagged_0(@name("s_tag") bit<12> s_tag, @name("c_tag") bit<12> c_tag) {
        hdr.vlan_tag_[1].setValid();
        hdr.vlan_tag_[0].setValid();
        hdr.vlan_tag_[1].etherType = hdr.ethernet.etherType;
        hdr.vlan_tag_[1].vid = c_tag;
        hdr.vlan_tag_[0].etherType = 16w0x8100;
        hdr.vlan_tag_[0].vid = s_tag;
        hdr.ethernet.etherType = 16w0x9100;
    }
    @name(".egress_vlan_xlate") table _egress_vlan_xlate {
        actions = {
            _set_egress_packet_vlan_untagged_0();
            _set_egress_packet_vlan_tagged_0();
            _set_egress_packet_vlan_double_tagged_0();
            @defaultonly NoAction_33();
        }
        key = {
            meta.egress_metadata.ifindex: exact @name("egress_metadata.ifindex");
            meta.egress_metadata.bd     : exact @name("egress_metadata.bd");
        }
        size = 1024;
        default_action = NoAction_33();
    }
    @name(".egress_filter_check") action _egress_filter_check_0() {
        meta.egress_filter_metadata.ifindex_check = meta.ingress_metadata.ifindex ^ meta.egress_metadata.ifindex;
        meta.egress_filter_metadata.bd = meta.ingress_metadata.outer_bd ^ meta.egress_metadata.outer_bd;
        meta.egress_filter_metadata.inner_bd = meta.ingress_metadata.bd ^ meta.egress_metadata.bd;
    }
    @name(".set_egress_filter_drop") action _set_egress_filter_drop_0() {
        mark_to_drop(standard_metadata);
    }
    @name(".egress_filter") table _egress_filter {
        actions = {
            _egress_filter_check_0();
            @defaultonly NoAction_34();
        }
        default_action = NoAction_34();
    }
    @name(".egress_filter_drop") table _egress_filter_drop {
        actions = {
            _set_egress_filter_drop_0();
            @defaultonly NoAction_35();
        }
        default_action = NoAction_35();
    }
    @name(".nop") action _nop_57() {
    }
    @name(".egress_mirror") action _egress_mirror_0(@name("session_id") bit<32> session_id) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        clone_preserving_field_list(CloneType.E2E, session_id, 8w3);
    }
    @name(".egress_mirror_drop") action _egress_mirror_drop_0(@name("session_id") bit<32> session_id_6) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id_6;
        clone_preserving_field_list(CloneType.E2E, session_id_6, 8w3);
        mark_to_drop(standard_metadata);
    }
    @name(".egress_redirect_to_cpu") action _egress_redirect_to_cpu_0(@name("reason_code") bit<16> reason_code_0) {
        meta.fabric_metadata.reason_code = reason_code_0;
        clone_preserving_field_list(CloneType.E2E, 32w250, 8w2);
        mark_to_drop(standard_metadata);
    }
    @name(".egress_acl") table _egress_acl {
        actions = {
            _nop_57();
            _egress_mirror_0();
            _egress_mirror_drop_0();
            _egress_redirect_to_cpu_0();
            @defaultonly NoAction_36();
        }
        key = {
            standard_metadata.egress_port: ternary @name("standard_metadata.egress_port");
            meta.l3_metadata.l3_mtu_check: ternary @name("l3_metadata.l3_mtu_check");
        }
        size = 512;
        default_action = NoAction_36();
    }
    apply {
        if (meta.egress_metadata.bypass == 1w0) {
            if (standard_metadata.instance_type != 32w0 && standard_metadata.instance_type != 32w5) {
                mirror_0.apply();
            } else if (standard_metadata.egress_rid != 16w0) {
                _rid.apply();
                _replica_type.apply();
            }
            switch (egress_port_mapping_0.apply().action_run) {
                egress_port_type_normal: {
                    if (standard_metadata.instance_type == 32w0 || standard_metadata.instance_type == 32w5) {
                        _vlan_decap.apply();
                    }
                    if (meta.tunnel_metadata.tunnel_terminate == 1w1) {
                        if (meta.multicast_metadata.inner_replica == 1w1 || meta.multicast_metadata.replica == 1w0) {
                            _tunnel_decap_process_outer.apply();
                            _tunnel_decap_process_inner.apply();
                        }
                    }
                    if (meta.egress_metadata.routed == 1w0 || meta.l3_metadata.nexthop_index != 16w0) {
                        _rewrite.apply();
                    } else {
                        _rewrite_multicast.apply();
                    }
                    _egress_bd_map.apply();
                    if (meta.egress_metadata.routed == 1w1) {
                        _l3_rewrite.apply();
                        _smac_rewrite.apply();
                    }
                    _mtu.apply();
                    switch (_int_insert.apply().action_run) {
                        _int_transit_0: {
                            if (meta.int_metadata.insert_cnt != 8w0) {
                                _int_inst.apply();
                                _int_inst_0.apply();
                                _int_inst_1.apply();
                                _int_inst_2.apply();
                                _int_bos.apply();
                            }
                            _int_meta_header_update.apply();
                        }
                        default: {
                        }
                    }
                    _egress_bd_stats_0.apply();
                }
                default: {
                }
            }
            if (meta.fabric_metadata.fabric_header_present == 1w0 && meta.tunnel_metadata.egress_tunnel_type != 5w0) {
                _egress_vni.apply();
                if (meta.tunnel_metadata.egress_tunnel_type != 5w15 && meta.tunnel_metadata.egress_tunnel_type != 5w16) {
                    _tunnel_encap_process_inner.apply();
                }
                _tunnel_encap_process_outer.apply();
                _tunnel_rewrite.apply();
                _tunnel_mtu.apply();
                _tunnel_src_rewrite.apply();
                _tunnel_dst_rewrite.apply();
                _tunnel_smac_rewrite.apply();
                _tunnel_dmac_rewrite.apply();
            }
            if (meta.int_metadata.insert_cnt != 8w0) {
                _int_outer_encap.apply();
            }
            if (meta.egress_metadata.port_type == 2w0) {
                _egress_vlan_xlate.apply();
            }
            _egress_filter.apply();
            if (meta.multicast_metadata.inner_replica == 1w1) {
                if (meta.tunnel_metadata.ingress_tunnel_type == 5w0 && meta.tunnel_metadata.egress_tunnel_type == 5w0 && meta.egress_filter_metadata.bd == 16w0 && meta.egress_filter_metadata.ifindex_check == 16w0 || meta.tunnel_metadata.ingress_tunnel_type != 5w0 && meta.tunnel_metadata.egress_tunnel_type != 5w0 && meta.egress_filter_metadata.inner_bd == 16w0) {
                    _egress_filter_drop.apply();
                }
            }
        }
        if (meta.egress_metadata.bypass == 1w0) {
            _egress_acl.apply();
        }
    }
}

@name(".storm_control_meter") meter<bit<10>>(32w1024, MeterType.bytes) storm_control_meter;
@name(".ingress_bd_stats_count") @min_width(32) counter<bit<10>>(32w1024, CounterType.packets_and_bytes) ingress_bd_stats_count;
@name(".acl_stats_count") @min_width(16) counter<bit<10>>(32w1024, CounterType.packets_and_bytes) acl_stats_count;
@name("mac_learn_digest") struct mac_learn_digest {
    bit<16> bd;
    bit<48> lkp_mac_sa;
    bit<16> ifindex;
}

@name(".drop_stats") counter<bit<10>>(32w1024, CounterType.packets) drop_stats;
@name(".drop_stats_2") counter<bit<10>>(32w1024, CounterType.packets) drop_stats_2;
control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_37() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_38() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_39() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_40() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_41() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_42() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_43() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_44() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_45() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_46() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_47() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_48() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_49() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_50() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_51() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_52() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_53() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_54() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_55() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_56() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_57() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_58() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_59() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_60() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_61() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_62() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_63() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_64() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_65() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_66() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_67() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_68() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_69() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_70() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_71() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_72() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_73() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_74() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_75() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_76() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_77() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_78() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_79() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_80() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_81() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_82() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_83() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_84() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_85() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_86() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_87() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_88() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_89() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_90() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_91() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_92() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_93() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_94() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_95() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_96() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_97() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_98() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_99() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_100() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_101() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_102() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_103() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_104() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_105() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_106() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_107() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_108() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_109() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_110() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_111() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_112() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_113() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_114() {
    }
    @name(".rmac_hit") action rmac_hit_1() {
        meta.l3_metadata.rmac_hit = 1w1;
    }
    @name(".rmac_miss") action rmac_miss() {
        meta.l3_metadata.rmac_hit = 1w0;
    }
    @name(".rmac") table rmac_0 {
        actions = {
            rmac_hit_1();
            rmac_miss();
            @defaultonly NoAction_37();
        }
        key = {
            meta.l3_metadata.rmac_group: exact @name("l3_metadata.rmac_group");
            meta.l2_metadata.lkp_mac_da: exact @name("l2_metadata.lkp_mac_da");
        }
        size = 1024;
        default_action = NoAction_37();
    }
    @name(".set_ifindex") action _set_ifindex_0(@name("ifindex") bit<16> ifindex_14, @name("port_type") bit<2> port_type_1) {
        meta.ingress_metadata.ifindex = ifindex_14;
        meta.ingress_metadata.port_type = port_type_1;
    }
    @name(".set_ingress_port_properties") action _set_ingress_port_properties_0(@name("if_label") bit<16> if_label_1) {
        meta.acl_metadata.if_label = if_label_1;
    }
    @name(".ingress_port_mapping") table _ingress_port_mapping {
        actions = {
            _set_ifindex_0();
            @defaultonly NoAction_38();
        }
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port");
        }
        size = 288;
        default_action = NoAction_38();
    }
    @name(".ingress_port_properties") table _ingress_port_properties {
        actions = {
            _set_ingress_port_properties_0();
            @defaultonly NoAction_39();
        }
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port");
        }
        size = 288;
        default_action = NoAction_39();
    }
    @name(".malformed_outer_ethernet_packet") action _malformed_outer_ethernet_packet_0(@name("drop_reason") bit<8> drop_reason_5) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason_5;
    }
    @name(".set_valid_outer_unicast_packet_untagged") action _set_valid_outer_unicast_packet_untagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_unicast_packet_single_tagged") action _set_valid_outer_unicast_packet_single_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
    }
    @name(".set_valid_outer_unicast_packet_double_tagged") action _set_valid_outer_unicast_packet_double_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
    }
    @name(".set_valid_outer_unicast_packet_qinq_tagged") action _set_valid_outer_unicast_packet_qinq_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_multicast_packet_untagged") action _set_valid_outer_multicast_packet_untagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_multicast_packet_single_tagged") action _set_valid_outer_multicast_packet_single_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
    }
    @name(".set_valid_outer_multicast_packet_double_tagged") action _set_valid_outer_multicast_packet_double_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
    }
    @name(".set_valid_outer_multicast_packet_qinq_tagged") action _set_valid_outer_multicast_packet_qinq_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_broadcast_packet_untagged") action _set_valid_outer_broadcast_packet_untagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_broadcast_packet_single_tagged") action _set_valid_outer_broadcast_packet_single_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
    }
    @name(".set_valid_outer_broadcast_packet_double_tagged") action _set_valid_outer_broadcast_packet_double_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
    }
    @name(".set_valid_outer_broadcast_packet_qinq_tagged") action _set_valid_outer_broadcast_packet_qinq_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".validate_outer_ethernet") table _validate_outer_ethernet {
        actions = {
            _malformed_outer_ethernet_packet_0();
            _set_valid_outer_unicast_packet_untagged_0();
            _set_valid_outer_unicast_packet_single_tagged_0();
            _set_valid_outer_unicast_packet_double_tagged_0();
            _set_valid_outer_unicast_packet_qinq_tagged_0();
            _set_valid_outer_multicast_packet_untagged_0();
            _set_valid_outer_multicast_packet_single_tagged_0();
            _set_valid_outer_multicast_packet_double_tagged_0();
            _set_valid_outer_multicast_packet_qinq_tagged_0();
            _set_valid_outer_broadcast_packet_untagged_0();
            _set_valid_outer_broadcast_packet_single_tagged_0();
            _set_valid_outer_broadcast_packet_double_tagged_0();
            _set_valid_outer_broadcast_packet_qinq_tagged_0();
            @defaultonly NoAction_40();
        }
        key = {
            hdr.ethernet.srcAddr      : ternary @name("ethernet.srcAddr");
            hdr.ethernet.dstAddr      : ternary @name("ethernet.dstAddr");
            hdr.vlan_tag_[0].isValid(): exact @name("vlan_tag_[0].$valid$");
            hdr.vlan_tag_[1].isValid(): exact @name("vlan_tag_[1].$valid$");
        }
        size = 512;
        default_action = NoAction_40();
    }
    @name(".set_valid_outer_ipv4_packet") action _set_valid_outer_ipv4_packet() {
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.l3_metadata.lkp_ip_tc = hdr.ipv4.diffserv;
        meta.l3_metadata.lkp_ip_version = hdr.ipv4.version;
    }
    @name(".set_malformed_outer_ipv4_packet") action _set_malformed_outer_ipv4_packet(@name("drop_reason") bit<8> drop_reason_6) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason_6;
    }
    @name(".validate_outer_ipv4_packet") table _validate_outer_ipv4_packet_0 {
        actions = {
            _set_valid_outer_ipv4_packet();
            _set_malformed_outer_ipv4_packet();
            @defaultonly NoAction_41();
        }
        key = {
            hdr.ipv4.version       : ternary @name("ipv4.version");
            hdr.ipv4.ttl           : ternary @name("ipv4.ttl");
            hdr.ipv4.srcAddr[31:24]: ternary @name("ipv4.srcAddr");
        }
        size = 512;
        default_action = NoAction_41();
    }
    @name(".set_valid_outer_ipv6_packet") action _set_valid_outer_ipv6_packet() {
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.l3_metadata.lkp_ip_tc = hdr.ipv6.trafficClass;
        meta.l3_metadata.lkp_ip_version = hdr.ipv6.version;
    }
    @name(".set_malformed_outer_ipv6_packet") action _set_malformed_outer_ipv6_packet(@name("drop_reason") bit<8> drop_reason_7) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason_7;
    }
    @name(".validate_outer_ipv6_packet") table _validate_outer_ipv6_packet_0 {
        actions = {
            _set_valid_outer_ipv6_packet();
            _set_malformed_outer_ipv6_packet();
            @defaultonly NoAction_42();
        }
        key = {
            hdr.ipv6.version         : ternary @name("ipv6.version");
            hdr.ipv6.hopLimit        : ternary @name("ipv6.hopLimit");
            hdr.ipv6.srcAddr[127:112]: ternary @name("ipv6.srcAddr");
        }
        size = 512;
        default_action = NoAction_42();
    }
    @name(".set_valid_mpls_label1") action _set_valid_mpls_label1() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[0].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[0].exp;
    }
    @name(".set_valid_mpls_label2") action _set_valid_mpls_label2() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[1].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[1].exp;
    }
    @name(".set_valid_mpls_label3") action _set_valid_mpls_label3() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[2].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[2].exp;
    }
    @name(".validate_mpls_packet") table _validate_mpls_packet_0 {
        actions = {
            _set_valid_mpls_label1();
            _set_valid_mpls_label2();
            _set_valid_mpls_label3();
            @defaultonly NoAction_43();
        }
        key = {
            hdr.mpls[0].label    : ternary @name("mpls[0].label");
            hdr.mpls[0].bos      : ternary @name("mpls[0].bos");
            hdr.mpls[0].isValid(): exact @name("mpls[0].$valid$");
            hdr.mpls[1].label    : ternary @name("mpls[1].label");
            hdr.mpls[1].bos      : ternary @name("mpls[1].bos");
            hdr.mpls[1].isValid(): exact @name("mpls[1].$valid$");
            hdr.mpls[2].label    : ternary @name("mpls[2].label");
            hdr.mpls[2].bos      : ternary @name("mpls[2].bos");
            hdr.mpls[2].isValid(): exact @name("mpls[2].$valid$");
        }
        size = 512;
        default_action = NoAction_43();
    }
    @name(".set_config_parameters") action _set_config_parameters_0(@name("enable_dod") bit<8> enable_dod_0) {
        meta.i2e_metadata.ingress_tstamp = (bit<32>)standard_metadata.ingress_global_timestamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
        standard_metadata.egress_spec = 9w511;
        random<bit<32>>(meta.ingress_metadata.sflow_take_sample, 32w0, 32w0x7fffffff);
    }
    @name(".switch_config_params") table _switch_config_params {
        actions = {
            _set_config_parameters_0();
            @defaultonly NoAction_44();
        }
        size = 1;
        default_action = NoAction_44();
    }
    @name(".set_bd_properties") action _set_bd_properties_0(@name("bd") bit<16> bd_24, @name("vrf") bit<16> vrf_7, @name("stp_group") bit<10> stp_group_1, @name("learning_enabled") bit<1> learning_enabled_1, @name("bd_label") bit<16> bd_label_4, @name("stats_idx") bit<16> stats_idx, @name("rmac_group") bit<10> rmac_group_5, @name("ipv4_unicast_enabled") bit<1> ipv4_unicast_enabled_3, @name("ipv6_unicast_enabled") bit<1> ipv6_unicast_enabled_3, @name("ipv4_urpf_mode") bit<2> ipv4_urpf_mode_3, @name("ipv6_urpf_mode") bit<2> ipv6_urpf_mode_3, @name("igmp_snooping_enabled") bit<1> igmp_snooping_enabled_2, @name("mld_snooping_enabled") bit<1> mld_snooping_enabled_2, @name("ipv4_multicast_enabled") bit<1> ipv4_multicast_enabled_3, @name("ipv6_multicast_enabled") bit<1> ipv6_multicast_enabled_3, @name("mrpf_group") bit<16> mrpf_group, @name("ipv4_mcast_key") bit<16> ipv4_mcast_key_1, @name("ipv4_mcast_key_type") bit<1> ipv4_mcast_key_type_1, @name("ipv6_mcast_key") bit<16> ipv6_mcast_key_1, @name("ipv6_mcast_key_type") bit<1> ipv6_mcast_key_type_1) {
        meta.ingress_metadata.bd = bd_24;
        meta.ingress_metadata.outer_bd = bd_24;
        meta.acl_metadata.bd_label = bd_label_4;
        meta.l2_metadata.stp_group = stp_group_1;
        meta.l2_metadata.bd_stats_idx = stats_idx;
        meta.l2_metadata.learning_enabled = learning_enabled_1;
        meta.l3_metadata.vrf = vrf_7;
        meta.ipv4_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled_3;
        meta.ipv6_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled_3;
        meta.ipv4_metadata.ipv4_urpf_mode = ipv4_urpf_mode_3;
        meta.ipv6_metadata.ipv6_urpf_mode = ipv6_urpf_mode_3;
        meta.l3_metadata.rmac_group = rmac_group_5;
        meta.multicast_metadata.igmp_snooping_enabled = igmp_snooping_enabled_2;
        meta.multicast_metadata.mld_snooping_enabled = mld_snooping_enabled_2;
        meta.multicast_metadata.ipv4_multicast_enabled = ipv4_multicast_enabled_3;
        meta.multicast_metadata.ipv6_multicast_enabled = ipv6_multicast_enabled_3;
        meta.multicast_metadata.bd_mrpf_group = mrpf_group;
        meta.multicast_metadata.ipv4_mcast_key_type = ipv4_mcast_key_type_1;
        meta.multicast_metadata.ipv4_mcast_key = ipv4_mcast_key_1;
        meta.multicast_metadata.ipv6_mcast_key_type = ipv6_mcast_key_type_1;
        meta.multicast_metadata.ipv6_mcast_key = ipv6_mcast_key_1;
    }
    @name(".port_vlan_mapping_miss") action _port_vlan_mapping_miss_0() {
        meta.l2_metadata.port_vlan_mapping_miss = 1w1;
    }
    @name(".port_vlan_mapping") table _port_vlan_mapping {
        actions = {
            _set_bd_properties_0();
            _port_vlan_mapping_miss_0();
            @defaultonly NoAction_45();
        }
        key = {
            meta.ingress_metadata.ifindex: exact @name("ingress_metadata.ifindex");
            hdr.vlan_tag_[0].isValid()   : exact @name("vlan_tag_[0].$valid$");
            hdr.vlan_tag_[0].vid         : exact @name("vlan_tag_[0].vid");
            hdr.vlan_tag_[1].isValid()   : exact @name("vlan_tag_[1].$valid$");
            hdr.vlan_tag_[1].vid         : exact @name("vlan_tag_[1].vid");
        }
        size = 4096;
        implementation = bd_action_profile;
        default_action = NoAction_45();
    }
    @name(".set_stp_state") action _set_stp_state_0(@name("stp_state") bit<3> stp_state_1) {
        meta.l2_metadata.stp_state = stp_state_1;
    }
    @name(".spanning_tree") table _spanning_tree {
        actions = {
            _set_stp_state_0();
            @defaultonly NoAction_46();
        }
        key = {
            meta.ingress_metadata.ifindex: exact @name("ingress_metadata.ifindex");
            meta.l2_metadata.stp_group   : exact @name("l2_metadata.stp_group");
        }
        size = 1024;
        default_action = NoAction_46();
    }
    @name(".on_miss") action _on_miss() {
    }
    @name(".ipsg_miss") action _ipsg_miss_0() {
        meta.security_metadata.ipsg_check_fail = 1w1;
    }
    @name(".ipsg") table _ipsg {
        actions = {
            _on_miss();
            @defaultonly NoAction_47();
        }
        key = {
            meta.ingress_metadata.ifindex : exact @name("ingress_metadata.ifindex");
            meta.ingress_metadata.bd      : exact @name("ingress_metadata.bd");
            meta.l2_metadata.lkp_mac_sa   : exact @name("l2_metadata.lkp_mac_sa");
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("ipv4_metadata.lkp_ipv4_sa");
        }
        size = 1024;
        default_action = NoAction_47();
    }
    @name(".ipsg_permit_special") table _ipsg_permit_special {
        actions = {
            _ipsg_miss_0();
            @defaultonly NoAction_48();
        }
        key = {
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto");
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport");
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da");
        }
        size = 512;
        default_action = NoAction_48();
    }
    @name(".int_sink_update_vxlan_gpe_v4") action _int_sink_update_vxlan_gpe_v4_0() {
        hdr.vxlan_gpe.next_proto = hdr.vxlan_gpe_int_header.next_proto;
        hdr.vxlan_gpe_int_header.setInvalid();
        hdr.ipv4.totalLen = hdr.ipv4.totalLen - meta.int_metadata.insert_byte_cnt;
        hdr.udp.length_ = hdr.udp.length_ - meta.int_metadata.insert_byte_cnt;
    }
    @name(".nop") action _nop_58() {
    }
    @name(".int_set_src") action _int_set_src_0() {
        meta.int_metadata_i2e.source = 1w1;
    }
    @name(".int_set_no_src") action _int_set_no_src_0() {
        meta.int_metadata_i2e.source = 1w0;
    }
    @name(".int_sink_gpe") action _int_sink_gpe_0(@name("mirror_id") bit<32> mirror_id) {
        meta.int_metadata.insert_byte_cnt = meta.int_metadata.gpe_int_hdr_len << 2;
        meta.int_metadata_i2e.sink = 1w1;
        meta.i2e_metadata.mirror_session_id = (bit<16>)mirror_id;
        clone_preserving_field_list(CloneType.I2E, mirror_id, 8w4);
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
    @name(".int_no_sink") action _int_no_sink_0() {
        meta.int_metadata_i2e.sink = 1w0;
    }
    @name(".int_sink_update_outer") table _int_sink_update_outer {
        actions = {
            _int_sink_update_vxlan_gpe_v4_0();
            _nop_58();
            @defaultonly NoAction_49();
        }
        key = {
            hdr.vxlan_gpe_int_header.isValid(): exact @name("vxlan_gpe_int_header.$valid$");
            hdr.ipv4.isValid()                : exact @name("ipv4.$valid$");
            meta.int_metadata_i2e.sink        : exact @name("int_metadata_i2e.sink");
        }
        size = 2;
        default_action = NoAction_49();
    }
    @name(".int_source") table _int_source {
        actions = {
            _int_set_src_0();
            _int_set_no_src_0();
            @defaultonly NoAction_50();
        }
        key = {
            hdr.int_header.isValid()      : exact @name("int_header.$valid$");
            hdr.ipv4.isValid()            : exact @name("ipv4.$valid$");
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da");
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("ipv4_metadata.lkp_ipv4_sa");
            hdr.inner_ipv4.isValid()      : exact @name("inner_ipv4.$valid$");
            hdr.inner_ipv4.dstAddr        : ternary @name("inner_ipv4.dstAddr");
            hdr.inner_ipv4.srcAddr        : ternary @name("inner_ipv4.srcAddr");
        }
        size = 256;
        default_action = NoAction_50();
    }
    @name(".int_terminate") table _int_terminate {
        actions = {
            _int_sink_gpe_0();
            _int_no_sink_0();
            @defaultonly NoAction_51();
        }
        key = {
            hdr.int_header.isValid()          : exact @name("int_header.$valid$");
            hdr.vxlan_gpe_int_header.isValid(): exact @name("vxlan_gpe_int_header.$valid$");
            hdr.ipv4.isValid()                : exact @name("ipv4.$valid$");
            meta.ipv4_metadata.lkp_ipv4_da    : ternary @name("ipv4_metadata.lkp_ipv4_da");
            hdr.inner_ipv4.isValid()          : exact @name("inner_ipv4.$valid$");
            hdr.inner_ipv4.dstAddr            : ternary @name("inner_ipv4.dstAddr");
        }
        size = 256;
        default_action = NoAction_51();
    }
    @name(".on_miss") action _on_miss_0() {
    }
    @name(".outer_rmac_hit") action _outer_rmac_hit_0() {
        meta.l3_metadata.rmac_hit = 1w1;
    }
    @name(".nop") action _nop_59() {
    }
    @name(".tunnel_lookup_miss") action _tunnel_lookup_miss_0() {
    }
    @name(".terminate_tunnel_inner_non_ip") action _terminate_tunnel_inner_non_ip_0(@name("bd") bit<16> bd_25, @name("bd_label") bit<16> bd_label_5, @name("stats_idx") bit<16> stats_idx_4) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.ingress_metadata.bd = bd_25;
        meta.acl_metadata.bd_label = bd_label_5;
        meta.l2_metadata.bd_stats_idx = stats_idx_4;
        meta.l3_metadata.lkp_ip_type = 2w0;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_tunnel_inner_ethernet_ipv4") action _terminate_tunnel_inner_ethernet_ipv4_0(@name("bd") bit<16> bd_26, @name("vrf") bit<16> vrf_8, @name("rmac_group") bit<10> rmac_group_6, @name("bd_label") bit<16> bd_label_6, @name("ipv4_unicast_enabled") bit<1> ipv4_unicast_enabled_4, @name("ipv4_urpf_mode") bit<2> ipv4_urpf_mode_4, @name("igmp_snooping_enabled") bit<1> igmp_snooping_enabled_3, @name("stats_idx") bit<16> stats_idx_5, @name("ipv4_multicast_enabled") bit<1> ipv4_multicast_enabled_4, @name("mrpf_group") bit<16> mrpf_group_5) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.ingress_metadata.bd = bd_26;
        meta.l3_metadata.vrf = vrf_8;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.ipv4_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled_4;
        meta.ipv4_metadata.ipv4_urpf_mode = ipv4_urpf_mode_4;
        meta.l3_metadata.rmac_group = rmac_group_6;
        meta.acl_metadata.bd_label = bd_label_6;
        meta.l2_metadata.bd_stats_idx = stats_idx_5;
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
        meta.multicast_metadata.igmp_snooping_enabled = igmp_snooping_enabled_3;
        meta.multicast_metadata.ipv4_multicast_enabled = ipv4_multicast_enabled_4;
        meta.multicast_metadata.bd_mrpf_group = mrpf_group_5;
    }
    @name(".terminate_tunnel_inner_ipv4") action _terminate_tunnel_inner_ipv4_0(@name("vrf") bit<16> vrf_9, @name("rmac_group") bit<10> rmac_group_7, @name("ipv4_urpf_mode") bit<2> ipv4_urpf_mode_5, @name("ipv4_unicast_enabled") bit<1> ipv4_unicast_enabled_5, @name("ipv4_multicast_enabled") bit<1> ipv4_multicast_enabled_5, @name("mrpf_group") bit<16> mrpf_group_6) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.l3_metadata.vrf = vrf_9;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.ipv4_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled_5;
        meta.ipv4_metadata.ipv4_urpf_mode = ipv4_urpf_mode_5;
        meta.l3_metadata.rmac_group = rmac_group_7;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
        meta.multicast_metadata.bd_mrpf_group = mrpf_group_6;
        meta.multicast_metadata.ipv4_multicast_enabled = ipv4_multicast_enabled_5;
    }
    @name(".terminate_tunnel_inner_ethernet_ipv6") action _terminate_tunnel_inner_ethernet_ipv6_0(@name("bd") bit<16> bd_27, @name("vrf") bit<16> vrf_10, @name("rmac_group") bit<10> rmac_group_8, @name("bd_label") bit<16> bd_label_7, @name("ipv6_unicast_enabled") bit<1> ipv6_unicast_enabled_4, @name("ipv6_urpf_mode") bit<2> ipv6_urpf_mode_4, @name("mld_snooping_enabled") bit<1> mld_snooping_enabled_3, @name("stats_idx") bit<16> stats_idx_6, @name("ipv6_multicast_enabled") bit<1> ipv6_multicast_enabled_4, @name("mrpf_group") bit<16> mrpf_group_7) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.ingress_metadata.bd = bd_27;
        meta.l3_metadata.vrf = vrf_10;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.ipv6_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled_4;
        meta.ipv6_metadata.ipv6_urpf_mode = ipv6_urpf_mode_4;
        meta.l3_metadata.rmac_group = rmac_group_8;
        meta.acl_metadata.bd_label = bd_label_7;
        meta.l2_metadata.bd_stats_idx = stats_idx_6;
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv6.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv6.trafficClass;
        meta.multicast_metadata.bd_mrpf_group = mrpf_group_7;
        meta.multicast_metadata.ipv6_multicast_enabled = ipv6_multicast_enabled_4;
        meta.multicast_metadata.mld_snooping_enabled = mld_snooping_enabled_3;
    }
    @name(".terminate_tunnel_inner_ipv6") action _terminate_tunnel_inner_ipv6_0(@name("vrf") bit<16> vrf_11, @name("rmac_group") bit<10> rmac_group_9, @name("ipv6_unicast_enabled") bit<1> ipv6_unicast_enabled_5, @name("ipv6_urpf_mode") bit<2> ipv6_urpf_mode_5, @name("ipv6_multicast_enabled") bit<1> ipv6_multicast_enabled_5, @name("mrpf_group") bit<16> mrpf_group_8) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.l3_metadata.vrf = vrf_11;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.ipv6_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled_5;
        meta.ipv6_metadata.ipv6_urpf_mode = ipv6_urpf_mode_5;
        meta.l3_metadata.rmac_group = rmac_group_9;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv6.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv6.trafficClass;
        meta.multicast_metadata.bd_mrpf_group = mrpf_group_8;
        meta.multicast_metadata.ipv6_multicast_enabled = ipv6_multicast_enabled_5;
    }
    @name(".non_ip_tunnel_lookup_miss") action _non_ip_tunnel_lookup_miss_0() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        standard_metadata.mcast_grp = 16w0;
    }
    @name(".non_ip_tunnel_lookup_miss") action _non_ip_tunnel_lookup_miss_1() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        standard_metadata.mcast_grp = 16w0;
    }
    @name(".ipv4_tunnel_lookup_miss") action _ipv4_tunnel_lookup_miss_0() {
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
    @name(".ipv4_tunnel_lookup_miss") action _ipv4_tunnel_lookup_miss_1() {
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
    @name(".ipv6_tunnel_lookup_miss") action _ipv6_tunnel_lookup_miss_0() {
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
    @name(".ipv6_tunnel_lookup_miss") action _ipv6_tunnel_lookup_miss_1() {
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
    @name(".outer_rmac") table _outer_rmac {
        actions = {
            _on_miss_0();
            _outer_rmac_hit_0();
            @defaultonly NoAction_52();
        }
        key = {
            meta.l3_metadata.rmac_group: exact @name("l3_metadata.rmac_group");
            hdr.ethernet.dstAddr       : exact @name("ethernet.dstAddr");
        }
        size = 1024;
        default_action = NoAction_52();
    }
    @name(".tunnel") table _tunnel {
        actions = {
            _nop_59();
            _tunnel_lookup_miss_0();
            _terminate_tunnel_inner_non_ip_0();
            _terminate_tunnel_inner_ethernet_ipv4_0();
            _terminate_tunnel_inner_ipv4_0();
            _terminate_tunnel_inner_ethernet_ipv6_0();
            _terminate_tunnel_inner_ipv6_0();
            @defaultonly NoAction_53();
        }
        key = {
            meta.tunnel_metadata.tunnel_vni         : exact @name("tunnel_metadata.tunnel_vni");
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type");
            hdr.inner_ipv4.isValid()                : exact @name("inner_ipv4.$valid$");
            hdr.inner_ipv6.isValid()                : exact @name("inner_ipv6.$valid$");
        }
        size = 1024;
        default_action = NoAction_53();
    }
    @name(".tunnel_lookup_miss") table _tunnel_lookup_miss_1 {
        actions = {
            _non_ip_tunnel_lookup_miss_0();
            _ipv4_tunnel_lookup_miss_0();
            _ipv6_tunnel_lookup_miss_0();
            @defaultonly NoAction_54();
        }
        key = {
            hdr.ipv4.isValid(): exact @name("ipv4.$valid$");
            hdr.ipv6.isValid(): exact @name("ipv6.$valid$");
        }
        default_action = NoAction_54();
    }
    @name(".tunnel_miss") table _tunnel_miss {
        actions = {
            _non_ip_tunnel_lookup_miss_1();
            _ipv4_tunnel_lookup_miss_1();
            _ipv6_tunnel_lookup_miss_1();
            @defaultonly NoAction_55();
        }
        key = {
            hdr.ipv4.isValid(): exact @name("ipv4.$valid$");
            hdr.ipv6.isValid(): exact @name("ipv6.$valid$");
        }
        default_action = NoAction_55();
    }
    @name(".nop") action _nop_60() {
    }
    @name(".nop") action _nop_61() {
    }
    @name(".terminate_cpu_packet") action _terminate_cpu_packet() {
        standard_metadata.egress_spec = (bit<9>)hdr.fabric_header.dstPortOrGroup;
        meta.egress_metadata.bypass = hdr.fabric_header_cpu.txBypass;
        hdr.ethernet.etherType = hdr.fabric_payload_header.etherType;
        hdr.fabric_header.setInvalid();
        hdr.fabric_header_cpu.setInvalid();
        hdr.fabric_payload_header.setInvalid();
    }
    @name(".switch_fabric_unicast_packet") action _switch_fabric_unicast_packet() {
        meta.fabric_metadata.fabric_header_present = 1w1;
        meta.fabric_metadata.dst_device = hdr.fabric_header.dstDevice;
        meta.fabric_metadata.dst_port = hdr.fabric_header.dstPortOrGroup;
    }
    @name(".terminate_fabric_unicast_packet") action _terminate_fabric_unicast_packet() {
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
    @name(".switch_fabric_multicast_packet") action _switch_fabric_multicast_packet() {
        meta.fabric_metadata.fabric_header_present = 1w1;
        standard_metadata.mcast_grp = hdr.fabric_header.dstPortOrGroup;
    }
    @name(".terminate_fabric_multicast_packet") action _terminate_fabric_multicast_packet() {
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
    @name(".set_ingress_ifindex_properties") action _set_ingress_ifindex_properties() {
    }
    @name(".non_ip_over_fabric") action _non_ip_over_fabric() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".ipv4_over_fabric") action _ipv4_over_fabric() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.ipv4.protocol;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_outer_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_outer_l4_dport;
    }
    @name(".ipv6_over_fabric") action _ipv6_over_fabric() {
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.ipv6.nextHdr;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_outer_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_outer_l4_dport;
    }
    @name(".fabric_ingress_dst_lkp") table _fabric_ingress_dst_lkp_0 {
        actions = {
            _nop_60();
            _terminate_cpu_packet();
            _switch_fabric_unicast_packet();
            _terminate_fabric_unicast_packet();
            _switch_fabric_multicast_packet();
            _terminate_fabric_multicast_packet();
            @defaultonly NoAction_56();
        }
        key = {
            hdr.fabric_header.dstDevice: exact @name("fabric_header.dstDevice");
        }
        default_action = NoAction_56();
    }
    @name(".fabric_ingress_src_lkp") table _fabric_ingress_src_lkp_0 {
        actions = {
            _nop_61();
            _set_ingress_ifindex_properties();
            @defaultonly NoAction_57();
        }
        key = {
            hdr.fabric_header_multicast.ingressIfindex: exact @name("fabric_header_multicast.ingressIfindex");
        }
        size = 1024;
        default_action = NoAction_57();
    }
    @name(".native_packet_over_fabric") table _native_packet_over_fabric_0 {
        actions = {
            _non_ip_over_fabric();
            _ipv4_over_fabric();
            _ipv6_over_fabric();
            @defaultonly NoAction_58();
        }
        key = {
            hdr.ipv4.isValid(): exact @name("ipv4.$valid$");
            hdr.ipv6.isValid(): exact @name("ipv6.$valid$");
        }
        size = 1024;
        default_action = NoAction_58();
    }
    @name(".nop") action _nop_62() {
    }
    @name(".nop") action _nop_63() {
    }
    @name(".on_miss") action _on_miss_1() {
    }
    @name(".outer_multicast_route_s_g_hit") action _outer_multicast_route_s_g_hit(@name("mc_index") bit<16> mc_index, @name("mcast_rpf_group") bit<16> mcast_rpf_group_12) {
        standard_metadata.mcast_grp = mc_index;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_12 ^ meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_bridge_s_g_hit") action _outer_multicast_bridge_s_g_hit(@name("mc_index") bit<16> mc_index_22) {
        standard_metadata.mcast_grp = mc_index_22;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_route_sm_star_g_hit") action _outer_multicast_route_sm_star_g_hit(@name("mc_index") bit<16> mc_index_23, @name("mcast_rpf_group") bit<16> mcast_rpf_group_13) {
        meta.multicast_metadata.outer_mcast_mode = 2w1;
        standard_metadata.mcast_grp = mc_index_23;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_13 ^ meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_route_bidir_star_g_hit") action _outer_multicast_route_bidir_star_g_hit(@name("mc_index") bit<16> mc_index_24, @name("mcast_rpf_group") bit<16> mcast_rpf_group_14) {
        meta.multicast_metadata.outer_mcast_mode = 2w2;
        standard_metadata.mcast_grp = mc_index_24;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_14 | meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_bridge_star_g_hit") action _outer_multicast_bridge_star_g_hit(@name("mc_index") bit<16> mc_index_25) {
        standard_metadata.mcast_grp = mc_index_25;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_ipv4_multicast") table _outer_ipv4_multicast_0 {
        actions = {
            _nop_62();
            _on_miss_1();
            _outer_multicast_route_s_g_hit();
            _outer_multicast_bridge_s_g_hit();
            @defaultonly NoAction_59();
        }
        key = {
            meta.multicast_metadata.ipv4_mcast_key_type: exact @name("multicast_metadata.ipv4_mcast_key_type");
            meta.multicast_metadata.ipv4_mcast_key     : exact @name("multicast_metadata.ipv4_mcast_key");
            hdr.ipv4.srcAddr                           : exact @name("ipv4.srcAddr");
            hdr.ipv4.dstAddr                           : exact @name("ipv4.dstAddr");
        }
        size = 1024;
        default_action = NoAction_59();
    }
    @name(".outer_ipv4_multicast_star_g") table _outer_ipv4_multicast_star_g_0 {
        actions = {
            _nop_63();
            _outer_multicast_route_sm_star_g_hit();
            _outer_multicast_route_bidir_star_g_hit();
            _outer_multicast_bridge_star_g_hit();
            @defaultonly NoAction_60();
        }
        key = {
            meta.multicast_metadata.ipv4_mcast_key_type: exact @name("multicast_metadata.ipv4_mcast_key_type");
            meta.multicast_metadata.ipv4_mcast_key     : exact @name("multicast_metadata.ipv4_mcast_key");
            hdr.ipv4.dstAddr                           : ternary @name("ipv4.dstAddr");
        }
        size = 512;
        default_action = NoAction_60();
    }
    @name(".nop") action _nop_64() {
    }
    @name(".nop") action _nop_65() {
    }
    @name(".on_miss") action _on_miss_2() {
    }
    @name(".outer_multicast_route_s_g_hit") action _outer_multicast_route_s_g_hit_0(@name("mc_index") bit<16> mc_index_26, @name("mcast_rpf_group") bit<16> mcast_rpf_group_15) {
        standard_metadata.mcast_grp = mc_index_26;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_15 ^ meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_bridge_s_g_hit") action _outer_multicast_bridge_s_g_hit_0(@name("mc_index") bit<16> mc_index_27) {
        standard_metadata.mcast_grp = mc_index_27;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_route_sm_star_g_hit") action _outer_multicast_route_sm_star_g_hit_0(@name("mc_index") bit<16> mc_index_28, @name("mcast_rpf_group") bit<16> mcast_rpf_group_16) {
        meta.multicast_metadata.outer_mcast_mode = 2w1;
        standard_metadata.mcast_grp = mc_index_28;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_16 ^ meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_route_bidir_star_g_hit") action _outer_multicast_route_bidir_star_g_hit_0(@name("mc_index") bit<16> mc_index_29, @name("mcast_rpf_group") bit<16> mcast_rpf_group_17) {
        meta.multicast_metadata.outer_mcast_mode = 2w2;
        standard_metadata.mcast_grp = mc_index_29;
        meta.multicast_metadata.outer_mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_17 | meta.multicast_metadata.bd_mrpf_group;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_multicast_bridge_star_g_hit") action _outer_multicast_bridge_star_g_hit_0(@name("mc_index") bit<16> mc_index_30) {
        standard_metadata.mcast_grp = mc_index_30;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".outer_ipv6_multicast") table _outer_ipv6_multicast_0 {
        actions = {
            _nop_64();
            _on_miss_2();
            _outer_multicast_route_s_g_hit_0();
            _outer_multicast_bridge_s_g_hit_0();
            @defaultonly NoAction_61();
        }
        key = {
            meta.multicast_metadata.ipv6_mcast_key_type: exact @name("multicast_metadata.ipv6_mcast_key_type");
            meta.multicast_metadata.ipv6_mcast_key     : exact @name("multicast_metadata.ipv6_mcast_key");
            hdr.ipv6.srcAddr                           : exact @name("ipv6.srcAddr");
            hdr.ipv6.dstAddr                           : exact @name("ipv6.dstAddr");
        }
        size = 1024;
        default_action = NoAction_61();
    }
    @name(".outer_ipv6_multicast_star_g") table _outer_ipv6_multicast_star_g_0 {
        actions = {
            _nop_65();
            _outer_multicast_route_sm_star_g_hit_0();
            _outer_multicast_route_bidir_star_g_hit_0();
            _outer_multicast_bridge_star_g_hit_0();
            @defaultonly NoAction_62();
        }
        key = {
            meta.multicast_metadata.ipv6_mcast_key_type: exact @name("multicast_metadata.ipv6_mcast_key_type");
            meta.multicast_metadata.ipv6_mcast_key     : exact @name("multicast_metadata.ipv6_mcast_key");
            hdr.ipv6.dstAddr                           : ternary @name("ipv6.dstAddr");
        }
        size = 512;
        default_action = NoAction_62();
    }
    @name(".nop") action _nop_66() {
    }
    @name(".set_tunnel_termination_flag") action _set_tunnel_termination_flag() {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".set_tunnel_vni_and_termination_flag") action _set_tunnel_vni_and_termination_flag(@name("tunnel_vni") bit<24> tunnel_vni_2) {
        meta.tunnel_metadata.tunnel_vni = tunnel_vni_2;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".on_miss") action _on_miss_3() {
    }
    @name(".src_vtep_hit") action _src_vtep_hit(@name("ifindex") bit<16> ifindex_15) {
        meta.ingress_metadata.ifindex = ifindex_15;
    }
    @name(".ipv4_dest_vtep") table _ipv4_dest_vtep_0 {
        actions = {
            _nop_66();
            _set_tunnel_termination_flag();
            _set_tunnel_vni_and_termination_flag();
            @defaultonly NoAction_63();
        }
        key = {
            meta.l3_metadata.vrf                    : exact @name("l3_metadata.vrf");
            hdr.ipv4.dstAddr                        : exact @name("ipv4.dstAddr");
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type");
        }
        size = 1024;
        default_action = NoAction_63();
    }
    @name(".ipv4_src_vtep") table _ipv4_src_vtep_0 {
        actions = {
            _on_miss_3();
            _src_vtep_hit();
            @defaultonly NoAction_64();
        }
        key = {
            meta.l3_metadata.vrf                    : exact @name("l3_metadata.vrf");
            hdr.ipv4.srcAddr                        : exact @name("ipv4.srcAddr");
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type");
        }
        size = 1024;
        default_action = NoAction_64();
    }
    @name(".nop") action _nop_67() {
    }
    @name(".set_tunnel_termination_flag") action _set_tunnel_termination_flag_0() {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".set_tunnel_vni_and_termination_flag") action _set_tunnel_vni_and_termination_flag_0(@name("tunnel_vni") bit<24> tunnel_vni_3) {
        meta.tunnel_metadata.tunnel_vni = tunnel_vni_3;
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".on_miss") action _on_miss_4() {
    }
    @name(".src_vtep_hit") action _src_vtep_hit_0(@name("ifindex") bit<16> ifindex_16) {
        meta.ingress_metadata.ifindex = ifindex_16;
    }
    @name(".ipv6_dest_vtep") table _ipv6_dest_vtep_0 {
        actions = {
            _nop_67();
            _set_tunnel_termination_flag_0();
            _set_tunnel_vni_and_termination_flag_0();
            @defaultonly NoAction_65();
        }
        key = {
            meta.l3_metadata.vrf                    : exact @name("l3_metadata.vrf");
            hdr.ipv6.dstAddr                        : exact @name("ipv6.dstAddr");
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type");
        }
        size = 1024;
        default_action = NoAction_65();
    }
    @name(".ipv6_src_vtep") table _ipv6_src_vtep_0 {
        actions = {
            _on_miss_4();
            _src_vtep_hit_0();
            @defaultonly NoAction_66();
        }
        key = {
            meta.l3_metadata.vrf                    : exact @name("l3_metadata.vrf");
            hdr.ipv6.srcAddr                        : exact @name("ipv6.srcAddr");
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type");
        }
        size = 1024;
        default_action = NoAction_66();
    }
    @name(".terminate_eompls") action _terminate_eompls(@name("bd") bit<16> bd_28, @name("tunnel_type") bit<5> tunnel_type_11) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type_11;
        meta.ingress_metadata.bd = bd_28;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_vpls") action _terminate_vpls(@name("bd") bit<16> bd_29, @name("tunnel_type") bit<5> tunnel_type_12) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type_12;
        meta.ingress_metadata.bd = bd_29;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_ipv4_over_mpls") action _terminate_ipv4_over_mpls(@name("vrf") bit<16> vrf_12, @name("tunnel_type") bit<5> tunnel_type_13) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type_13;
        meta.l3_metadata.vrf = vrf_12;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
    }
    @name(".terminate_ipv6_over_mpls") action _terminate_ipv6_over_mpls(@name("vrf") bit<16> vrf_13, @name("tunnel_type") bit<5> tunnel_type_14) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type_14;
        meta.l3_metadata.vrf = vrf_13;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv6.version;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv6.trafficClass;
    }
    @name(".terminate_pw") action _terminate_pw(@name("ifindex") bit<16> ifindex_17) {
        meta.ingress_metadata.egress_ifindex = ifindex_17;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
    }
    @name(".forward_mpls") action _forward_mpls(@name("nexthop_index") bit<16> nexthop_index_8) {
        meta.l3_metadata.fib_nexthop = nexthop_index_8;
        meta.l3_metadata.fib_nexthop_type = 1w0;
        meta.l3_metadata.fib_hit = 1w1;
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
    }
    @name(".mpls") table _mpls_0 {
        actions = {
            _terminate_eompls();
            _terminate_vpls();
            _terminate_ipv4_over_mpls();
            _terminate_ipv6_over_mpls();
            _terminate_pw();
            _forward_mpls();
            @defaultonly NoAction_67();
        }
        key = {
            meta.tunnel_metadata.mpls_label: exact @name("tunnel_metadata.mpls_label");
            hdr.inner_ipv4.isValid()       : exact @name("inner_ipv4.$valid$");
            hdr.inner_ipv6.isValid()       : exact @name("inner_ipv6.$valid$");
        }
        size = 1024;
        default_action = NoAction_67();
    }
    @name(".sflow_ingress_session_pkt_counter") direct_counter(CounterType.packets) _sflow_ingress_session_pkt_counter;
    @name(".nop") action _nop_68() {
    }
    @name(".sflow_ing_session_enable") action _sflow_ing_session_enable_0(@name("rate_thr") bit<32> rate_thr, @name("session_id") bit<16> session_id_7) {
        meta.ingress_metadata.sflow_take_sample = rate_thr |+| meta.ingress_metadata.sflow_take_sample;
        meta.sflow_metadata.sflow_session_id = session_id_7;
    }
    @name(".nop") action _nop_69() {
        _sflow_ingress_session_pkt_counter.count();
    }
    @name(".sflow_ing_pkt_to_cpu") action _sflow_ing_pkt_to_cpu_0(@name("sflow_i2e_mirror_id") bit<32> sflow_i2e_mirror_id, @name("reason_code") bit<16> reason_code_5) {
        _sflow_ingress_session_pkt_counter.count();
        meta.fabric_metadata.reason_code = reason_code_5;
        meta.i2e_metadata.mirror_session_id = (bit<16>)sflow_i2e_mirror_id;
        clone_preserving_field_list(CloneType.I2E, sflow_i2e_mirror_id, 8w6);
    }
    @name(".sflow_ing_take_sample") table _sflow_ing_take_sample {
        actions = {
            _nop_69();
            _sflow_ing_pkt_to_cpu_0();
            @defaultonly NoAction_68();
        }
        key = {
            meta.ingress_metadata.sflow_take_sample: ternary @name("ingress_metadata.sflow_take_sample");
            meta.sflow_metadata.sflow_session_id   : exact @name("sflow_metadata.sflow_session_id");
        }
        counters = _sflow_ingress_session_pkt_counter;
        default_action = NoAction_68();
    }
    @name(".sflow_ingress") table _sflow_ingress {
        actions = {
            _nop_68();
            _sflow_ing_session_enable_0();
            @defaultonly NoAction_69();
        }
        key = {
            meta.ingress_metadata.ifindex : ternary @name("ingress_metadata.ifindex");
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("ipv4_metadata.lkp_ipv4_sa");
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da");
            hdr.sflow.isValid()           : exact @name("sflow.$valid$");
        }
        size = 512;
        default_action = NoAction_69();
    }
    @name(".nop") action _nop_70() {
    }
    @name(".set_storm_control_meter") action _set_storm_control_meter_0(@name("meter_idx") bit<16> meter_idx) {
        storm_control_meter.execute_meter<bit<2>>((bit<10>)meter_idx, meta.meter_metadata.meter_color);
        meta.meter_metadata.meter_index = meter_idx;
    }
    @name(".storm_control") table _storm_control {
        actions = {
            _nop_70();
            _set_storm_control_meter_0();
            @defaultonly NoAction_70();
        }
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port");
            meta.l2_metadata.lkp_pkt_type : ternary @name("l2_metadata.lkp_pkt_type");
        }
        size = 512;
        default_action = NoAction_70();
    }
    @name(".nop") action _nop_71() {
    }
    @name(".set_unicast") action _set_unicast_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
    }
    @name(".set_unicast_and_ipv6_src_is_link_local") action _set_unicast_and_ipv6_src_is_link_local_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.ipv6_metadata.ipv6_src_is_link_local = 1w1;
    }
    @name(".set_multicast") action _set_multicast_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w1;
    }
    @name(".set_multicast_and_ipv6_src_is_link_local") action _set_multicast_and_ipv6_src_is_link_local_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.ipv6_metadata.ipv6_src_is_link_local = 1w1;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w1;
    }
    @name(".set_broadcast") action _set_broadcast_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w2;
    }
    @name(".set_malformed_packet") action _set_malformed_packet_0(@name("drop_reason") bit<8> drop_reason_8) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason_8;
    }
    @name(".validate_packet") table _validate_packet {
        actions = {
            _nop_71();
            _set_unicast_0();
            _set_unicast_and_ipv6_src_is_link_local_0();
            _set_multicast_0();
            _set_multicast_and_ipv6_src_is_link_local_0();
            _set_broadcast_0();
            _set_malformed_packet_0();
            @defaultonly NoAction_71();
        }
        key = {
            meta.l2_metadata.lkp_mac_sa[40:40]     : ternary @name("l2_metadata.lkp_mac_sa");
            meta.l2_metadata.lkp_mac_da            : ternary @name("l2_metadata.lkp_mac_da");
            meta.l3_metadata.lkp_ip_type           : ternary @name("l3_metadata.lkp_ip_type");
            meta.l3_metadata.lkp_ip_ttl            : ternary @name("l3_metadata.lkp_ip_ttl");
            meta.l3_metadata.lkp_ip_version        : ternary @name("l3_metadata.lkp_ip_version");
            meta.ipv4_metadata.lkp_ipv4_sa[31:24]  : ternary @name("ipv4_metadata.lkp_ipv4_sa");
            meta.ipv6_metadata.lkp_ipv6_sa[127:112]: ternary @name("ipv6_metadata.lkp_ipv6_sa");
        }
        size = 512;
        default_action = NoAction_71();
    }
    @name(".nop") action _nop_72() {
    }
    @name(".nop") action _nop_73() {
    }
    @name(".dmac_hit") action _dmac_hit_0(@name("ifindex") bit<16> ifindex_18) {
        meta.ingress_metadata.egress_ifindex = ifindex_18;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex_18;
    }
    @name(".dmac_multicast_hit") action _dmac_multicast_hit_0(@name("mc_index") bit<16> mc_index_31) {
        standard_metadata.mcast_grp = mc_index_31;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".dmac_miss") action _dmac_miss_0() {
        meta.ingress_metadata.egress_ifindex = 16w65535;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".dmac_redirect_nexthop") action _dmac_redirect_nexthop_0(@name("nexthop_index") bit<16> nexthop_index_9) {
        meta.l2_metadata.l2_redirect = 1w1;
        meta.l2_metadata.l2_nexthop = nexthop_index_9;
        meta.l2_metadata.l2_nexthop_type = 1w0;
    }
    @name(".dmac_redirect_ecmp") action _dmac_redirect_ecmp_0(@name("ecmp_index") bit<16> ecmp_index) {
        meta.l2_metadata.l2_redirect = 1w1;
        meta.l2_metadata.l2_nexthop = ecmp_index;
        meta.l2_metadata.l2_nexthop_type = 1w1;
    }
    @name(".dmac_drop") action _dmac_drop_0() {
        mark_to_drop(standard_metadata);
    }
    @name(".smac_miss") action _smac_miss_0() {
        meta.l2_metadata.l2_src_miss = 1w1;
    }
    @name(".smac_hit") action _smac_hit_0(@name("ifindex") bit<16> ifindex_19) {
        meta.l2_metadata.l2_src_move = meta.ingress_metadata.ifindex ^ ifindex_19;
    }
    @name(".dmac") table _dmac {
        support_timeout = true;
        actions = {
            _nop_72();
            _dmac_hit_0();
            _dmac_multicast_hit_0();
            _dmac_miss_0();
            _dmac_redirect_nexthop_0();
            _dmac_redirect_ecmp_0();
            _dmac_drop_0();
            @defaultonly NoAction_72();
        }
        key = {
            meta.ingress_metadata.bd   : exact @name("ingress_metadata.bd");
            meta.l2_metadata.lkp_mac_da: exact @name("l2_metadata.lkp_mac_da");
        }
        size = 1024;
        default_action = NoAction_72();
    }
    @name(".smac") table _smac {
        actions = {
            _nop_73();
            _smac_miss_0();
            _smac_hit_0();
            @defaultonly NoAction_73();
        }
        key = {
            meta.ingress_metadata.bd   : exact @name("ingress_metadata.bd");
            meta.l2_metadata.lkp_mac_sa: exact @name("l2_metadata.lkp_mac_sa");
        }
        size = 1024;
        default_action = NoAction_73();
    }
    @name(".nop") action _nop_74() {
    }
    @name(".acl_deny") action _acl_deny_1(@name("acl_stats_index") bit<14> acl_stats_index_18, @name("acl_meter_index") bit<16> acl_meter_index, @name("acl_copy") bit<1> acl_copy_16, @name("acl_copy_reason") bit<16> acl_copy_reason) {
        meta.acl_metadata.acl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index_18;
        meta.meter_metadata.meter_index = acl_meter_index;
        meta.acl_metadata.acl_copy = acl_copy_16;
        meta.fabric_metadata.reason_code = acl_copy_reason;
    }
    @name(".acl_permit") action _acl_permit_1(@name("acl_stats_index") bit<14> acl_stats_index_19, @name("acl_meter_index") bit<16> acl_meter_index_10, @name("acl_copy") bit<1> acl_copy_17, @name("acl_copy_reason") bit<16> acl_copy_reason_16) {
        meta.acl_metadata.acl_stats_index = acl_stats_index_19;
        meta.meter_metadata.meter_index = acl_meter_index_10;
        meta.acl_metadata.acl_copy = acl_copy_17;
        meta.fabric_metadata.reason_code = acl_copy_reason_16;
    }
    @name(".acl_mirror") action _acl_mirror_1(@name("session_id") bit<32> session_id_8, @name("acl_stats_index") bit<14> acl_stats_index_20, @name("acl_meter_index") bit<16> acl_meter_index_11) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id_8;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)standard_metadata.ingress_global_timestamp;
        clone_preserving_field_list(CloneType.I2E, session_id_8, 8w1);
        meta.acl_metadata.acl_stats_index = acl_stats_index_20;
        meta.meter_metadata.meter_index = acl_meter_index_11;
    }
    @name(".acl_redirect_nexthop") action _acl_redirect_nexthop_1(@name("nexthop_index") bit<16> nexthop_index_10, @name("acl_stats_index") bit<14> acl_stats_index_21, @name("acl_meter_index") bit<16> acl_meter_index_12, @name("acl_copy") bit<1> acl_copy_18, @name("acl_copy_reason") bit<16> acl_copy_reason_17) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = nexthop_index_10;
        meta.acl_metadata.acl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index_21;
        meta.meter_metadata.meter_index = acl_meter_index_12;
        meta.acl_metadata.acl_copy = acl_copy_18;
        meta.fabric_metadata.reason_code = acl_copy_reason_17;
    }
    @name(".acl_redirect_ecmp") action _acl_redirect_ecmp_1(@name("ecmp_index") bit<16> ecmp_index_7, @name("acl_stats_index") bit<14> acl_stats_index_22, @name("acl_meter_index") bit<16> acl_meter_index_13, @name("acl_copy") bit<1> acl_copy_19, @name("acl_copy_reason") bit<16> acl_copy_reason_18) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = ecmp_index_7;
        meta.acl_metadata.acl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index_22;
        meta.meter_metadata.meter_index = acl_meter_index_13;
        meta.acl_metadata.acl_copy = acl_copy_19;
        meta.fabric_metadata.reason_code = acl_copy_reason_18;
    }
    @name(".mac_acl") table _mac_acl {
        actions = {
            _nop_74();
            _acl_deny_1();
            _acl_permit_1();
            _acl_mirror_1();
            _acl_redirect_nexthop_1();
            _acl_redirect_ecmp_1();
            @defaultonly NoAction_74();
        }
        key = {
            meta.acl_metadata.if_label   : ternary @name("acl_metadata.if_label");
            meta.acl_metadata.bd_label   : ternary @name("acl_metadata.bd_label");
            meta.l2_metadata.lkp_mac_sa  : ternary @name("l2_metadata.lkp_mac_sa");
            meta.l2_metadata.lkp_mac_da  : ternary @name("l2_metadata.lkp_mac_da");
            meta.l2_metadata.lkp_mac_type: ternary @name("l2_metadata.lkp_mac_type");
        }
        size = 512;
        default_action = NoAction_74();
    }
    @name(".nop") action _nop_75() {
    }
    @name(".nop") action _nop_76() {
    }
    @name(".acl_deny") action _acl_deny_2(@name("acl_stats_index") bit<14> acl_stats_index_23, @name("acl_meter_index") bit<16> acl_meter_index_14, @name("acl_copy") bit<1> acl_copy_20, @name("acl_copy_reason") bit<16> acl_copy_reason_19) {
        meta.acl_metadata.acl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index_23;
        meta.meter_metadata.meter_index = acl_meter_index_14;
        meta.acl_metadata.acl_copy = acl_copy_20;
        meta.fabric_metadata.reason_code = acl_copy_reason_19;
    }
    @name(".acl_deny") action _acl_deny_3(@name("acl_stats_index") bit<14> acl_stats_index_24, @name("acl_meter_index") bit<16> acl_meter_index_15, @name("acl_copy") bit<1> acl_copy_21, @name("acl_copy_reason") bit<16> acl_copy_reason_20) {
        meta.acl_metadata.acl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index_24;
        meta.meter_metadata.meter_index = acl_meter_index_15;
        meta.acl_metadata.acl_copy = acl_copy_21;
        meta.fabric_metadata.reason_code = acl_copy_reason_20;
    }
    @name(".acl_permit") action _acl_permit_2(@name("acl_stats_index") bit<14> acl_stats_index_25, @name("acl_meter_index") bit<16> acl_meter_index_16, @name("acl_copy") bit<1> acl_copy_22, @name("acl_copy_reason") bit<16> acl_copy_reason_21) {
        meta.acl_metadata.acl_stats_index = acl_stats_index_25;
        meta.meter_metadata.meter_index = acl_meter_index_16;
        meta.acl_metadata.acl_copy = acl_copy_22;
        meta.fabric_metadata.reason_code = acl_copy_reason_21;
    }
    @name(".acl_permit") action _acl_permit_3(@name("acl_stats_index") bit<14> acl_stats_index_26, @name("acl_meter_index") bit<16> acl_meter_index_17, @name("acl_copy") bit<1> acl_copy_23, @name("acl_copy_reason") bit<16> acl_copy_reason_22) {
        meta.acl_metadata.acl_stats_index = acl_stats_index_26;
        meta.meter_metadata.meter_index = acl_meter_index_17;
        meta.acl_metadata.acl_copy = acl_copy_23;
        meta.fabric_metadata.reason_code = acl_copy_reason_22;
    }
    @name(".acl_mirror") action _acl_mirror_2(@name("session_id") bit<32> session_id_9, @name("acl_stats_index") bit<14> acl_stats_index_27, @name("acl_meter_index") bit<16> acl_meter_index_18) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id_9;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)standard_metadata.ingress_global_timestamp;
        clone_preserving_field_list(CloneType.I2E, session_id_9, 8w1);
        meta.acl_metadata.acl_stats_index = acl_stats_index_27;
        meta.meter_metadata.meter_index = acl_meter_index_18;
    }
    @name(".acl_mirror") action _acl_mirror_3(@name("session_id") bit<32> session_id_10, @name("acl_stats_index") bit<14> acl_stats_index_28, @name("acl_meter_index") bit<16> acl_meter_index_19) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id_10;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)standard_metadata.ingress_global_timestamp;
        clone_preserving_field_list(CloneType.I2E, session_id_10, 8w1);
        meta.acl_metadata.acl_stats_index = acl_stats_index_28;
        meta.meter_metadata.meter_index = acl_meter_index_19;
    }
    @name(".acl_redirect_nexthop") action _acl_redirect_nexthop_2(@name("nexthop_index") bit<16> nexthop_index_11, @name("acl_stats_index") bit<14> acl_stats_index_29, @name("acl_meter_index") bit<16> acl_meter_index_20, @name("acl_copy") bit<1> acl_copy_24, @name("acl_copy_reason") bit<16> acl_copy_reason_23) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = nexthop_index_11;
        meta.acl_metadata.acl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index_29;
        meta.meter_metadata.meter_index = acl_meter_index_20;
        meta.acl_metadata.acl_copy = acl_copy_24;
        meta.fabric_metadata.reason_code = acl_copy_reason_23;
    }
    @name(".acl_redirect_nexthop") action _acl_redirect_nexthop_3(@name("nexthop_index") bit<16> nexthop_index_12, @name("acl_stats_index") bit<14> acl_stats_index_30, @name("acl_meter_index") bit<16> acl_meter_index_21, @name("acl_copy") bit<1> acl_copy_25, @name("acl_copy_reason") bit<16> acl_copy_reason_24) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = nexthop_index_12;
        meta.acl_metadata.acl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index_30;
        meta.meter_metadata.meter_index = acl_meter_index_21;
        meta.acl_metadata.acl_copy = acl_copy_25;
        meta.fabric_metadata.reason_code = acl_copy_reason_24;
    }
    @name(".acl_redirect_ecmp") action _acl_redirect_ecmp_2(@name("ecmp_index") bit<16> ecmp_index_8, @name("acl_stats_index") bit<14> acl_stats_index_31, @name("acl_meter_index") bit<16> acl_meter_index_22, @name("acl_copy") bit<1> acl_copy_26, @name("acl_copy_reason") bit<16> acl_copy_reason_25) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = ecmp_index_8;
        meta.acl_metadata.acl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index_31;
        meta.meter_metadata.meter_index = acl_meter_index_22;
        meta.acl_metadata.acl_copy = acl_copy_26;
        meta.fabric_metadata.reason_code = acl_copy_reason_25;
    }
    @name(".acl_redirect_ecmp") action _acl_redirect_ecmp_3(@name("ecmp_index") bit<16> ecmp_index_9, @name("acl_stats_index") bit<14> acl_stats_index_32, @name("acl_meter_index") bit<16> acl_meter_index_23, @name("acl_copy") bit<1> acl_copy_27, @name("acl_copy_reason") bit<16> acl_copy_reason_26) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = ecmp_index_9;
        meta.acl_metadata.acl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index_32;
        meta.meter_metadata.meter_index = acl_meter_index_23;
        meta.acl_metadata.acl_copy = acl_copy_27;
        meta.fabric_metadata.reason_code = acl_copy_reason_26;
    }
    @name(".ip_acl") table _ip_acl {
        actions = {
            _nop_75();
            _acl_deny_2();
            _acl_permit_2();
            _acl_mirror_2();
            _acl_redirect_nexthop_2();
            _acl_redirect_ecmp_2();
            @defaultonly NoAction_75();
        }
        key = {
            meta.acl_metadata.if_label    : ternary @name("acl_metadata.if_label");
            meta.acl_metadata.bd_label    : ternary @name("acl_metadata.bd_label");
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("ipv4_metadata.lkp_ipv4_sa");
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da");
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto");
            meta.l3_metadata.lkp_l4_sport : ternary @name("l3_metadata.lkp_l4_sport");
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport");
            hdr.tcp.flags                 : ternary @name("tcp.flags");
            meta.l3_metadata.lkp_ip_ttl   : ternary @name("l3_metadata.lkp_ip_ttl");
        }
        size = 512;
        default_action = NoAction_75();
    }
    @name(".ipv6_acl") table _ipv6_acl {
        actions = {
            _nop_76();
            _acl_deny_3();
            _acl_permit_3();
            _acl_mirror_3();
            _acl_redirect_nexthop_3();
            _acl_redirect_ecmp_3();
            @defaultonly NoAction_76();
        }
        key = {
            meta.acl_metadata.if_label    : ternary @name("acl_metadata.if_label");
            meta.acl_metadata.bd_label    : ternary @name("acl_metadata.bd_label");
            meta.ipv6_metadata.lkp_ipv6_sa: ternary @name("ipv6_metadata.lkp_ipv6_sa");
            meta.ipv6_metadata.lkp_ipv6_da: ternary @name("ipv6_metadata.lkp_ipv6_da");
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto");
            meta.l3_metadata.lkp_l4_sport : ternary @name("l3_metadata.lkp_l4_sport");
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport");
            hdr.tcp.flags                 : ternary @name("tcp.flags");
            meta.l3_metadata.lkp_ip_ttl   : ternary @name("l3_metadata.lkp_ip_ttl");
        }
        size = 512;
        default_action = NoAction_76();
    }
    @name(".nop") action _nop_77() {
    }
    @name(".apply_cos_marking") action _apply_cos_marking_0(@name("cos") bit<3> cos) {
        meta.qos_metadata.marked_cos = cos;
    }
    @name(".apply_dscp_marking") action _apply_dscp_marking_0(@name("dscp") bit<8> dscp) {
        meta.qos_metadata.marked_dscp = dscp;
    }
    @name(".apply_tc_marking") action _apply_tc_marking_0(@name("tc") bit<3> tc) {
        meta.qos_metadata.marked_exp = tc;
    }
    @name(".qos") table _qos {
        actions = {
            _nop_77();
            _apply_cos_marking_0();
            _apply_dscp_marking_0();
            _apply_tc_marking_0();
            @defaultonly NoAction_77();
        }
        key = {
            meta.acl_metadata.if_label    : ternary @name("acl_metadata.if_label");
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("ipv4_metadata.lkp_ipv4_sa");
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da");
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto");
            meta.l3_metadata.lkp_ip_tc    : ternary @name("l3_metadata.lkp_ip_tc");
            meta.tunnel_metadata.mpls_exp : ternary @name("tunnel_metadata.mpls_exp");
            meta.qos_metadata.outer_dscp  : ternary @name("qos_metadata.outer_dscp");
        }
        size = 512;
        default_action = NoAction_77();
    }
    @name(".ipv4_multicast_route_s_g_stats") direct_counter(CounterType.packets) _ipv4_multicast_route_s_g_stats_0;
    @name(".ipv4_multicast_route_star_g_stats") direct_counter(CounterType.packets) _ipv4_multicast_route_star_g_stats_0;
    @name(".on_miss") action _on_miss_5() {
    }
    @name(".multicast_bridge_s_g_hit") action _multicast_bridge_s_g_hit(@name("mc_index") bit<16> mc_index_32) {
        meta.multicast_metadata.multicast_bridge_mc_index = mc_index_32;
        meta.multicast_metadata.mcast_bridge_hit = 1w1;
    }
    @name(".nop") action _nop_78() {
    }
    @name(".multicast_bridge_star_g_hit") action _multicast_bridge_star_g_hit(@name("mc_index") bit<16> mc_index_33) {
        meta.multicast_metadata.multicast_bridge_mc_index = mc_index_33;
        meta.multicast_metadata.mcast_bridge_hit = 1w1;
    }
    @name(".ipv4_multicast_bridge") table _ipv4_multicast_bridge_0 {
        actions = {
            _on_miss_5();
            _multicast_bridge_s_g_hit();
            @defaultonly NoAction_78();
        }
        key = {
            meta.ingress_metadata.bd      : exact @name("ingress_metadata.bd");
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("ipv4_metadata.lkp_ipv4_sa");
            meta.ipv4_metadata.lkp_ipv4_da: exact @name("ipv4_metadata.lkp_ipv4_da");
        }
        size = 1024;
        default_action = NoAction_78();
    }
    @name(".ipv4_multicast_bridge_star_g") table _ipv4_multicast_bridge_star_g_0 {
        actions = {
            _nop_78();
            _multicast_bridge_star_g_hit();
            @defaultonly NoAction_79();
        }
        key = {
            meta.ingress_metadata.bd      : exact @name("ingress_metadata.bd");
            meta.ipv4_metadata.lkp_ipv4_da: exact @name("ipv4_metadata.lkp_ipv4_da");
        }
        size = 1024;
        default_action = NoAction_79();
    }
    @name(".on_miss") action _on_miss_6() {
        _ipv4_multicast_route_s_g_stats_0.count();
    }
    @name(".multicast_route_s_g_hit") action _multicast_route_s_g_hit(@name("mc_index") bit<16> mc_index_34, @name("mcast_rpf_group") bit<16> mcast_rpf_group_18) {
        _ipv4_multicast_route_s_g_stats_0.count();
        meta.multicast_metadata.multicast_route_mc_index = mc_index_34;
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_18 ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".ipv4_multicast_route") table _ipv4_multicast_route_0 {
        actions = {
            _on_miss_6();
            _multicast_route_s_g_hit();
            @defaultonly NoAction_80();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("ipv4_metadata.lkp_ipv4_sa");
            meta.ipv4_metadata.lkp_ipv4_da: exact @name("ipv4_metadata.lkp_ipv4_da");
        }
        size = 1024;
        counters = _ipv4_multicast_route_s_g_stats_0;
        default_action = NoAction_80();
    }
    @name(".multicast_route_star_g_miss") action _multicast_route_star_g_miss() {
        _ipv4_multicast_route_star_g_stats_0.count();
        meta.l3_metadata.l3_copy = 1w1;
    }
    @name(".multicast_route_sm_star_g_hit") action _multicast_route_sm_star_g_hit(@name("mc_index") bit<16> mc_index_35, @name("mcast_rpf_group") bit<16> mcast_rpf_group_19) {
        _ipv4_multicast_route_star_g_stats_0.count();
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.multicast_route_mc_index = mc_index_35;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_19 ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".multicast_route_bidir_star_g_hit") action _multicast_route_bidir_star_g_hit(@name("mc_index") bit<16> mc_index_36, @name("mcast_rpf_group") bit<16> mcast_rpf_group_20) {
        _ipv4_multicast_route_star_g_stats_0.count();
        meta.multicast_metadata.mcast_mode = 2w2;
        meta.multicast_metadata.multicast_route_mc_index = mc_index_36;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_20 | meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".ipv4_multicast_route_star_g") table _ipv4_multicast_route_star_g_0 {
        actions = {
            _multicast_route_star_g_miss();
            _multicast_route_sm_star_g_hit();
            _multicast_route_bidir_star_g_hit();
            @defaultonly NoAction_81();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv4_metadata.lkp_ipv4_da: exact @name("ipv4_metadata.lkp_ipv4_da");
        }
        size = 1024;
        counters = _ipv4_multicast_route_star_g_stats_0;
        default_action = NoAction_81();
    }
    @name(".ipv6_multicast_route_s_g_stats") direct_counter(CounterType.packets) _ipv6_multicast_route_s_g_stats_0;
    @name(".ipv6_multicast_route_star_g_stats") direct_counter(CounterType.packets) _ipv6_multicast_route_star_g_stats_0;
    @name(".on_miss") action _on_miss_7() {
    }
    @name(".multicast_bridge_s_g_hit") action _multicast_bridge_s_g_hit_0(@name("mc_index") bit<16> mc_index_37) {
        meta.multicast_metadata.multicast_bridge_mc_index = mc_index_37;
        meta.multicast_metadata.mcast_bridge_hit = 1w1;
    }
    @name(".nop") action _nop_79() {
    }
    @name(".multicast_bridge_star_g_hit") action _multicast_bridge_star_g_hit_0(@name("mc_index") bit<16> mc_index_38) {
        meta.multicast_metadata.multicast_bridge_mc_index = mc_index_38;
        meta.multicast_metadata.mcast_bridge_hit = 1w1;
    }
    @name(".ipv6_multicast_bridge") table _ipv6_multicast_bridge_0 {
        actions = {
            _on_miss_7();
            _multicast_bridge_s_g_hit_0();
            @defaultonly NoAction_82();
        }
        key = {
            meta.ingress_metadata.bd      : exact @name("ingress_metadata.bd");
            meta.ipv6_metadata.lkp_ipv6_sa: exact @name("ipv6_metadata.lkp_ipv6_sa");
            meta.ipv6_metadata.lkp_ipv6_da: exact @name("ipv6_metadata.lkp_ipv6_da");
        }
        size = 1024;
        default_action = NoAction_82();
    }
    @name(".ipv6_multicast_bridge_star_g") table _ipv6_multicast_bridge_star_g_0 {
        actions = {
            _nop_79();
            _multicast_bridge_star_g_hit_0();
            @defaultonly NoAction_83();
        }
        key = {
            meta.ingress_metadata.bd      : exact @name("ingress_metadata.bd");
            meta.ipv6_metadata.lkp_ipv6_da: exact @name("ipv6_metadata.lkp_ipv6_da");
        }
        size = 1024;
        default_action = NoAction_83();
    }
    @name(".on_miss") action _on_miss_8() {
        _ipv6_multicast_route_s_g_stats_0.count();
    }
    @name(".multicast_route_s_g_hit") action _multicast_route_s_g_hit_0(@name("mc_index") bit<16> mc_index_39, @name("mcast_rpf_group") bit<16> mcast_rpf_group_21) {
        _ipv6_multicast_route_s_g_stats_0.count();
        meta.multicast_metadata.multicast_route_mc_index = mc_index_39;
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_21 ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".ipv6_multicast_route") table _ipv6_multicast_route_0 {
        actions = {
            _on_miss_8();
            _multicast_route_s_g_hit_0();
            @defaultonly NoAction_84();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv6_metadata.lkp_ipv6_sa: exact @name("ipv6_metadata.lkp_ipv6_sa");
            meta.ipv6_metadata.lkp_ipv6_da: exact @name("ipv6_metadata.lkp_ipv6_da");
        }
        size = 1024;
        counters = _ipv6_multicast_route_s_g_stats_0;
        default_action = NoAction_84();
    }
    @name(".multicast_route_star_g_miss") action _multicast_route_star_g_miss_0() {
        _ipv6_multicast_route_star_g_stats_0.count();
        meta.l3_metadata.l3_copy = 1w1;
    }
    @name(".multicast_route_sm_star_g_hit") action _multicast_route_sm_star_g_hit_0(@name("mc_index") bit<16> mc_index_40, @name("mcast_rpf_group") bit<16> mcast_rpf_group_22) {
        _ipv6_multicast_route_star_g_stats_0.count();
        meta.multicast_metadata.mcast_mode = 2w1;
        meta.multicast_metadata.multicast_route_mc_index = mc_index_40;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_22 ^ meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".multicast_route_bidir_star_g_hit") action _multicast_route_bidir_star_g_hit_0(@name("mc_index") bit<16> mc_index_41, @name("mcast_rpf_group") bit<16> mcast_rpf_group_23) {
        _ipv6_multicast_route_star_g_stats_0.count();
        meta.multicast_metadata.mcast_mode = 2w2;
        meta.multicast_metadata.multicast_route_mc_index = mc_index_41;
        meta.multicast_metadata.mcast_route_hit = 1w1;
        meta.multicast_metadata.mcast_rpf_group = mcast_rpf_group_23 | meta.multicast_metadata.bd_mrpf_group;
    }
    @name(".ipv6_multicast_route_star_g") table _ipv6_multicast_route_star_g_0 {
        actions = {
            _multicast_route_star_g_miss_0();
            _multicast_route_sm_star_g_hit_0();
            _multicast_route_bidir_star_g_hit_0();
            @defaultonly NoAction_85();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv6_metadata.lkp_ipv6_da: exact @name("ipv6_metadata.lkp_ipv6_da");
        }
        size = 1024;
        counters = _ipv6_multicast_route_star_g_stats_0;
        default_action = NoAction_85();
    }
    @name(".nop") action _nop_80() {
    }
    @name(".racl_deny") action _racl_deny_1(@name("acl_stats_index") bit<14> acl_stats_index_33, @name("acl_copy") bit<1> acl_copy_28, @name("acl_copy_reason") bit<16> acl_copy_reason_27) {
        meta.acl_metadata.racl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index_33;
        meta.acl_metadata.acl_copy = acl_copy_28;
        meta.fabric_metadata.reason_code = acl_copy_reason_27;
    }
    @name(".racl_permit") action _racl_permit_1(@name("acl_stats_index") bit<14> acl_stats_index_34, @name("acl_copy") bit<1> acl_copy_29, @name("acl_copy_reason") bit<16> acl_copy_reason_28) {
        meta.acl_metadata.acl_stats_index = acl_stats_index_34;
        meta.acl_metadata.acl_copy = acl_copy_29;
        meta.fabric_metadata.reason_code = acl_copy_reason_28;
    }
    @name(".racl_redirect_nexthop") action _racl_redirect_nexthop_1(@name("nexthop_index") bit<16> nexthop_index_13, @name("acl_stats_index") bit<14> acl_stats_index_35, @name("acl_copy") bit<1> acl_copy_30, @name("acl_copy_reason") bit<16> acl_copy_reason_29) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = nexthop_index_13;
        meta.acl_metadata.racl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index_35;
        meta.acl_metadata.acl_copy = acl_copy_30;
        meta.fabric_metadata.reason_code = acl_copy_reason_29;
    }
    @name(".racl_redirect_ecmp") action _racl_redirect_ecmp_1(@name("ecmp_index") bit<16> ecmp_index_10, @name("acl_stats_index") bit<14> acl_stats_index_36, @name("acl_copy") bit<1> acl_copy_31, @name("acl_copy_reason") bit<16> acl_copy_reason_30) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = ecmp_index_10;
        meta.acl_metadata.racl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index_36;
        meta.acl_metadata.acl_copy = acl_copy_31;
        meta.fabric_metadata.reason_code = acl_copy_reason_30;
    }
    @name(".ipv4_racl") table _ipv4_racl {
        actions = {
            _nop_80();
            _racl_deny_1();
            _racl_permit_1();
            _racl_redirect_nexthop_1();
            _racl_redirect_ecmp_1();
            @defaultonly NoAction_86();
        }
        key = {
            meta.acl_metadata.bd_label    : ternary @name("acl_metadata.bd_label");
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("ipv4_metadata.lkp_ipv4_sa");
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da");
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto");
            meta.l3_metadata.lkp_l4_sport : ternary @name("l3_metadata.lkp_l4_sport");
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport");
        }
        size = 512;
        default_action = NoAction_86();
    }
    @name(".on_miss") action _on_miss_23() {
    }
    @name(".ipv4_urpf_hit") action _ipv4_urpf_hit_0(@name("urpf_bd_group") bit<16> urpf_bd_group_2) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group_2;
        meta.l3_metadata.urpf_mode = meta.ipv4_metadata.ipv4_urpf_mode;
    }
    @name(".ipv4_urpf_hit") action _ipv4_urpf_hit_1(@name("urpf_bd_group") bit<16> urpf_bd_group_3) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group_3;
        meta.l3_metadata.urpf_mode = meta.ipv4_metadata.ipv4_urpf_mode;
    }
    @name(".urpf_miss") action _urpf_miss_1() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".ipv4_urpf") table _ipv4_urpf {
        actions = {
            _on_miss_23();
            _ipv4_urpf_hit_0();
            @defaultonly NoAction_87();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("ipv4_metadata.lkp_ipv4_sa");
        }
        size = 1024;
        default_action = NoAction_87();
    }
    @name(".ipv4_urpf_lpm") table _ipv4_urpf_lpm {
        actions = {
            _ipv4_urpf_hit_1();
            _urpf_miss_1();
            @defaultonly NoAction_88();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv4_metadata.lkp_ipv4_sa: lpm @name("ipv4_metadata.lkp_ipv4_sa");
        }
        size = 512;
        default_action = NoAction_88();
    }
    @name(".on_miss") action _on_miss_24() {
    }
    @name(".on_miss") action _on_miss_25() {
    }
    @name(".fib_hit_nexthop") action _fib_hit_nexthop_1(@name("nexthop_index") bit<16> nexthop_index_14) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index_14;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_nexthop") action _fib_hit_nexthop_2(@name("nexthop_index") bit<16> nexthop_index_15) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index_15;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_ecmp") action _fib_hit_ecmp_1(@name("ecmp_index") bit<16> ecmp_index_11) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index_11;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".fib_hit_ecmp") action _fib_hit_ecmp_2(@name("ecmp_index") bit<16> ecmp_index_12) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index_12;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".ipv4_fib") table _ipv4_fib {
        actions = {
            _on_miss_24();
            _fib_hit_nexthop_1();
            _fib_hit_ecmp_1();
            @defaultonly NoAction_89();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv4_metadata.lkp_ipv4_da: exact @name("ipv4_metadata.lkp_ipv4_da");
        }
        size = 1024;
        default_action = NoAction_89();
    }
    @name(".ipv4_fib_lpm") table _ipv4_fib_lpm {
        actions = {
            _on_miss_25();
            _fib_hit_nexthop_2();
            _fib_hit_ecmp_2();
            @defaultonly NoAction_90();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv4_metadata.lkp_ipv4_da: lpm @name("ipv4_metadata.lkp_ipv4_da");
        }
        size = 512;
        default_action = NoAction_90();
    }
    @name(".nop") action _nop_81() {
    }
    @name(".racl_deny") action _racl_deny_2(@name("acl_stats_index") bit<14> acl_stats_index_37, @name("acl_copy") bit<1> acl_copy_32, @name("acl_copy_reason") bit<16> acl_copy_reason_31) {
        meta.acl_metadata.racl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index_37;
        meta.acl_metadata.acl_copy = acl_copy_32;
        meta.fabric_metadata.reason_code = acl_copy_reason_31;
    }
    @name(".racl_permit") action _racl_permit_2(@name("acl_stats_index") bit<14> acl_stats_index_38, @name("acl_copy") bit<1> acl_copy_33, @name("acl_copy_reason") bit<16> acl_copy_reason_32) {
        meta.acl_metadata.acl_stats_index = acl_stats_index_38;
        meta.acl_metadata.acl_copy = acl_copy_33;
        meta.fabric_metadata.reason_code = acl_copy_reason_32;
    }
    @name(".racl_redirect_nexthop") action _racl_redirect_nexthop_2(@name("nexthop_index") bit<16> nexthop_index_16, @name("acl_stats_index") bit<14> acl_stats_index_39, @name("acl_copy") bit<1> acl_copy_34, @name("acl_copy_reason") bit<16> acl_copy_reason_33) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = nexthop_index_16;
        meta.acl_metadata.racl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index_39;
        meta.acl_metadata.acl_copy = acl_copy_34;
        meta.fabric_metadata.reason_code = acl_copy_reason_33;
    }
    @name(".racl_redirect_ecmp") action _racl_redirect_ecmp_2(@name("ecmp_index") bit<16> ecmp_index_13, @name("acl_stats_index") bit<14> acl_stats_index_40, @name("acl_copy") bit<1> acl_copy_35, @name("acl_copy_reason") bit<16> acl_copy_reason_34) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = ecmp_index_13;
        meta.acl_metadata.racl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index_40;
        meta.acl_metadata.acl_copy = acl_copy_35;
        meta.fabric_metadata.reason_code = acl_copy_reason_34;
    }
    @name(".ipv6_racl") table _ipv6_racl {
        actions = {
            _nop_81();
            _racl_deny_2();
            _racl_permit_2();
            _racl_redirect_nexthop_2();
            _racl_redirect_ecmp_2();
            @defaultonly NoAction_91();
        }
        key = {
            meta.acl_metadata.bd_label    : ternary @name("acl_metadata.bd_label");
            meta.ipv6_metadata.lkp_ipv6_sa: ternary @name("ipv6_metadata.lkp_ipv6_sa");
            meta.ipv6_metadata.lkp_ipv6_da: ternary @name("ipv6_metadata.lkp_ipv6_da");
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto");
            meta.l3_metadata.lkp_l4_sport : ternary @name("l3_metadata.lkp_l4_sport");
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport");
        }
        size = 512;
        default_action = NoAction_91();
    }
    @name(".on_miss") action _on_miss_26() {
    }
    @name(".ipv6_urpf_hit") action _ipv6_urpf_hit_0(@name("urpf_bd_group") bit<16> urpf_bd_group_4) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group_4;
        meta.l3_metadata.urpf_mode = meta.ipv6_metadata.ipv6_urpf_mode;
    }
    @name(".ipv6_urpf_hit") action _ipv6_urpf_hit_1(@name("urpf_bd_group") bit<16> urpf_bd_group_5) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group_5;
        meta.l3_metadata.urpf_mode = meta.ipv6_metadata.ipv6_urpf_mode;
    }
    @name(".urpf_miss") action _urpf_miss_2() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".ipv6_urpf") table _ipv6_urpf {
        actions = {
            _on_miss_26();
            _ipv6_urpf_hit_0();
            @defaultonly NoAction_92();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv6_metadata.lkp_ipv6_sa: exact @name("ipv6_metadata.lkp_ipv6_sa");
        }
        size = 1024;
        default_action = NoAction_92();
    }
    @name(".ipv6_urpf_lpm") table _ipv6_urpf_lpm {
        actions = {
            _ipv6_urpf_hit_1();
            _urpf_miss_2();
            @defaultonly NoAction_93();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv6_metadata.lkp_ipv6_sa: lpm @name("ipv6_metadata.lkp_ipv6_sa");
        }
        size = 512;
        default_action = NoAction_93();
    }
    @name(".on_miss") action _on_miss_27() {
    }
    @name(".on_miss") action _on_miss_28() {
    }
    @name(".fib_hit_nexthop") action _fib_hit_nexthop_3(@name("nexthop_index") bit<16> nexthop_index_17) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index_17;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_nexthop") action _fib_hit_nexthop_4(@name("nexthop_index") bit<16> nexthop_index_18) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index_18;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_ecmp") action _fib_hit_ecmp_3(@name("ecmp_index") bit<16> ecmp_index_14) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index_14;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".fib_hit_ecmp") action _fib_hit_ecmp_4(@name("ecmp_index") bit<16> ecmp_index_15) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index_15;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".ipv6_fib") table _ipv6_fib {
        actions = {
            _on_miss_27();
            _fib_hit_nexthop_3();
            _fib_hit_ecmp_3();
            @defaultonly NoAction_94();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv6_metadata.lkp_ipv6_da: exact @name("ipv6_metadata.lkp_ipv6_da");
        }
        size = 1024;
        default_action = NoAction_94();
    }
    @name(".ipv6_fib_lpm") table _ipv6_fib_lpm {
        actions = {
            _on_miss_28();
            _fib_hit_nexthop_4();
            _fib_hit_ecmp_4();
            @defaultonly NoAction_95();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf");
            meta.ipv6_metadata.lkp_ipv6_da: lpm @name("ipv6_metadata.lkp_ipv6_da");
        }
        size = 512;
        default_action = NoAction_95();
    }
    @name(".nop") action _nop_82() {
    }
    @name(".urpf_bd_miss") action _urpf_bd_miss_0() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".urpf_bd") table _urpf_bd {
        actions = {
            _nop_82();
            _urpf_bd_miss_0();
            @defaultonly NoAction_96();
        }
        key = {
            meta.l3_metadata.urpf_bd_group: exact @name("l3_metadata.urpf_bd_group");
            meta.ingress_metadata.bd      : exact @name("ingress_metadata.bd");
        }
        size = 1024;
        default_action = NoAction_96();
    }
    @name(".meter_index") direct_meter<bit<2>>(MeterType.bytes) _meter_index;
    @name(".nop") action _nop_83() {
        _meter_index.read(meta.meter_metadata.meter_color);
    }
    @name(".meter_index") table _meter_index_0 {
        actions = {
            _nop_83();
            @defaultonly NoAction_97();
        }
        key = {
            meta.meter_metadata.meter_index: exact @name("meter_metadata.meter_index");
        }
        size = 1024;
        meters = _meter_index;
        default_action = NoAction_97();
    }
    @name(".compute_lkp_ipv4_hash") action _compute_lkp_ipv4_hash_0() {
        hash<bit<16>, bit<16>, tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>>, bit<32>>(meta.hash_metadata.hash1, HashAlgorithm.crc16, 16w0, { meta.ipv4_metadata.lkp_ipv4_sa, meta.ipv4_metadata.lkp_ipv4_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
        hash<bit<16>, bit<16>, tuple<bit<48>, bit<48>, bit<32>, bit<32>, bit<8>, bit<16>, bit<16>>, bit<32>>(meta.hash_metadata.hash2, HashAlgorithm.crc16, 16w0, { meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.ipv4_metadata.lkp_ipv4_sa, meta.ipv4_metadata.lkp_ipv4_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
    }
    @name(".compute_lkp_ipv6_hash") action _compute_lkp_ipv6_hash_0() {
        hash<bit<16>, bit<16>, tuple<bit<128>, bit<128>, bit<8>, bit<16>, bit<16>>, bit<32>>(meta.hash_metadata.hash1, HashAlgorithm.crc16, 16w0, { meta.ipv6_metadata.lkp_ipv6_sa, meta.ipv6_metadata.lkp_ipv6_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
        hash<bit<16>, bit<16>, tuple<bit<48>, bit<48>, bit<128>, bit<128>, bit<8>, bit<16>, bit<16>>, bit<32>>(meta.hash_metadata.hash2, HashAlgorithm.crc16, 16w0, { meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.ipv6_metadata.lkp_ipv6_sa, meta.ipv6_metadata.lkp_ipv6_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
    }
    @name(".compute_lkp_non_ip_hash") action _compute_lkp_non_ip_hash_0() {
        hash<bit<16>, bit<16>, tuple<bit<16>, bit<48>, bit<48>, bit<16>>, bit<32>>(meta.hash_metadata.hash2, HashAlgorithm.crc16, 16w0, { meta.ingress_metadata.ifindex, meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.l2_metadata.lkp_mac_type }, 32w65536);
    }
    @name(".computed_two_hashes") action _computed_two_hashes_0() {
        meta.hash_metadata.entropy_hash = meta.hash_metadata.hash2;
    }
    @name(".computed_one_hash") action _computed_one_hash_0() {
        meta.hash_metadata.hash1 = meta.hash_metadata.hash2;
        meta.hash_metadata.entropy_hash = meta.hash_metadata.hash2;
    }
    @name(".compute_ipv4_hashes") table _compute_ipv4_hashes {
        actions = {
            _compute_lkp_ipv4_hash_0();
            @defaultonly NoAction_98();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("ingress_metadata.drop_flag");
        }
        default_action = NoAction_98();
    }
    @name(".compute_ipv6_hashes") table _compute_ipv6_hashes {
        actions = {
            _compute_lkp_ipv6_hash_0();
            @defaultonly NoAction_99();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("ingress_metadata.drop_flag");
        }
        default_action = NoAction_99();
    }
    @name(".compute_non_ip_hashes") table _compute_non_ip_hashes {
        actions = {
            _compute_lkp_non_ip_hash_0();
            @defaultonly NoAction_100();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("ingress_metadata.drop_flag");
        }
        default_action = NoAction_100();
    }
    @name(".compute_other_hashes") table _compute_other_hashes {
        actions = {
            _computed_two_hashes_0();
            _computed_one_hash_0();
            @defaultonly NoAction_101();
        }
        key = {
            meta.hash_metadata.hash1: exact @name("hash_metadata.hash1");
        }
        default_action = NoAction_101();
    }
    @name(".meter_stats") direct_counter(CounterType.packets) _meter_stats;
    @name(".meter_permit") action _meter_permit_0() {
        _meter_stats.count();
    }
    @name(".meter_deny") action _meter_deny_0() {
        _meter_stats.count();
        mark_to_drop(standard_metadata);
    }
    @name(".meter_action") table _meter_action {
        actions = {
            _meter_permit_0();
            _meter_deny_0();
            @defaultonly NoAction_102();
        }
        key = {
            meta.meter_metadata.meter_color: exact @name("meter_metadata.meter_color");
            meta.meter_metadata.meter_index: exact @name("meter_metadata.meter_index");
        }
        size = 1024;
        counters = _meter_stats;
        default_action = NoAction_102();
    }
    @name(".update_ingress_bd_stats") action _update_ingress_bd_stats_0() {
        ingress_bd_stats_count.count((bit<10>)meta.l2_metadata.bd_stats_idx);
    }
    @name(".ingress_bd_stats") table _ingress_bd_stats {
        actions = {
            _update_ingress_bd_stats_0();
            @defaultonly NoAction_103();
        }
        size = 1024;
        default_action = NoAction_103();
    }
    @name(".acl_stats_update") action _acl_stats_update_0() {
        acl_stats_count.count((bit<10>)meta.acl_metadata.acl_stats_index);
    }
    @name(".acl_stats") table _acl_stats {
        actions = {
            _acl_stats_update_0();
            @defaultonly NoAction_104();
        }
        size = 1024;
        default_action = NoAction_104();
    }
    @name(".storm_control_stats") direct_counter(CounterType.packets) _storm_control_stats;
    @name(".nop") action _nop_84() {
        _storm_control_stats.count();
    }
    @name(".storm_control_stats") table _storm_control_stats_0 {
        actions = {
            _nop_84();
            @defaultonly NoAction_105();
        }
        key = {
            meta.meter_metadata.meter_color: exact @name("meter_metadata.meter_color");
            standard_metadata.ingress_port : exact @name("standard_metadata.ingress_port");
        }
        size = 1024;
        counters = _storm_control_stats;
        default_action = NoAction_105();
    }
    @name(".nop") action _nop_85() {
    }
    @name(".set_l2_redirect_action") action _set_l2_redirect_action_0() {
        meta.l3_metadata.nexthop_index = meta.l2_metadata.l2_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.l2_metadata.l2_nexthop_type;
        meta.ingress_metadata.egress_ifindex = 16w0;
        standard_metadata.mcast_grp = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_fib_redirect_action") action _set_fib_redirect_action_0() {
        meta.l3_metadata.nexthop_index = meta.l3_metadata.fib_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.l3_metadata.fib_nexthop_type;
        meta.l3_metadata.routed = 1w1;
        standard_metadata.mcast_grp = 16w0;
        meta.fabric_metadata.reason_code = 16w0x217;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_cpu_redirect_action") action _set_cpu_redirect_action_0() {
        meta.l3_metadata.routed = 1w0;
        standard_metadata.mcast_grp = 16w0;
        standard_metadata.egress_spec = 9w64;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_acl_redirect_action") action _set_acl_redirect_action_0() {
        meta.l3_metadata.nexthop_index = meta.acl_metadata.acl_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.acl_metadata.acl_nexthop_type;
        meta.ingress_metadata.egress_ifindex = 16w0;
        standard_metadata.mcast_grp = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_racl_redirect_action") action _set_racl_redirect_action_0() {
        meta.l3_metadata.nexthop_index = meta.acl_metadata.racl_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.acl_metadata.racl_nexthop_type;
        meta.l3_metadata.routed = 1w1;
        meta.ingress_metadata.egress_ifindex = 16w0;
        standard_metadata.mcast_grp = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_multicast_route_action") action _set_multicast_route_action_0() {
        meta.fabric_metadata.dst_device = 8w127;
        meta.ingress_metadata.egress_ifindex = 16w0;
        standard_metadata.mcast_grp = meta.multicast_metadata.multicast_route_mc_index;
        meta.l3_metadata.routed = 1w1;
        meta.l3_metadata.same_bd_check = 16w0xffff;
    }
    @name(".set_multicast_bridge_action") action _set_multicast_bridge_action_0() {
        meta.fabric_metadata.dst_device = 8w127;
        meta.ingress_metadata.egress_ifindex = 16w0;
        standard_metadata.mcast_grp = meta.multicast_metadata.multicast_bridge_mc_index;
    }
    @name(".set_multicast_flood") action _set_multicast_flood_0() {
        meta.fabric_metadata.dst_device = 8w127;
        meta.ingress_metadata.egress_ifindex = 16w65535;
    }
    @name(".set_multicast_drop") action _set_multicast_drop_0() {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = 8w44;
    }
    @name(".fwd_result") table _fwd_result {
        actions = {
            _nop_85();
            _set_l2_redirect_action_0();
            _set_fib_redirect_action_0();
            _set_cpu_redirect_action_0();
            _set_acl_redirect_action_0();
            _set_racl_redirect_action_0();
            _set_multicast_route_action_0();
            _set_multicast_bridge_action_0();
            _set_multicast_flood_0();
            _set_multicast_drop_0();
            @defaultonly NoAction_106();
        }
        key = {
            meta.l2_metadata.l2_redirect                 : ternary @name("l2_metadata.l2_redirect");
            meta.acl_metadata.acl_redirect               : ternary @name("acl_metadata.acl_redirect");
            meta.acl_metadata.racl_redirect              : ternary @name("acl_metadata.racl_redirect");
            meta.l3_metadata.rmac_hit                    : ternary @name("l3_metadata.rmac_hit");
            meta.l3_metadata.fib_hit                     : ternary @name("l3_metadata.fib_hit");
            meta.l2_metadata.lkp_pkt_type                : ternary @name("l2_metadata.lkp_pkt_type");
            meta.l3_metadata.lkp_ip_type                 : ternary @name("l3_metadata.lkp_ip_type");
            meta.multicast_metadata.igmp_snooping_enabled: ternary @name("multicast_metadata.igmp_snooping_enabled");
            meta.multicast_metadata.mld_snooping_enabled : ternary @name("multicast_metadata.mld_snooping_enabled");
            meta.multicast_metadata.mcast_route_hit      : ternary @name("multicast_metadata.mcast_route_hit");
            meta.multicast_metadata.mcast_bridge_hit     : ternary @name("multicast_metadata.mcast_bridge_hit");
            meta.multicast_metadata.mcast_rpf_group      : ternary @name("multicast_metadata.mcast_rpf_group");
            meta.multicast_metadata.mcast_mode           : ternary @name("multicast_metadata.mcast_mode");
        }
        size = 512;
        default_action = NoAction_106();
    }
    @name(".nop") action _nop_86() {
    }
    @name(".nop") action _nop_87() {
    }
    @name(".set_ecmp_nexthop_details") action _set_ecmp_nexthop_details_0(@name("ifindex") bit<16> ifindex_20, @name("bd") bit<16> bd_30, @name("nhop_index") bit<16> nhop_index, @name("tunnel") bit<1> tunnel) {
        meta.ingress_metadata.egress_ifindex = ifindex_20;
        meta.l3_metadata.nexthop_index = nhop_index;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd_30;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex_20;
        meta.tunnel_metadata.tunnel_if_check = meta.tunnel_metadata.tunnel_terminate ^ tunnel;
    }
    @name(".set_ecmp_nexthop_details_for_post_routed_flood") action _set_ecmp_nexthop_details_for_post_routed_flood_0(@name("bd") bit<16> bd_31, @name("uuc_mc_index") bit<16> uuc_mc_index, @name("nhop_index") bit<16> nhop_index_2) {
        standard_metadata.mcast_grp = uuc_mc_index;
        meta.l3_metadata.nexthop_index = nhop_index_2;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd_31;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".set_nexthop_details") action _set_nexthop_details_0(@name("ifindex") bit<16> ifindex_21, @name("bd") bit<16> bd_32, @name("tunnel") bit<1> tunnel_0) {
        meta.ingress_metadata.egress_ifindex = ifindex_21;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd_32;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex_21;
        meta.tunnel_metadata.tunnel_if_check = meta.tunnel_metadata.tunnel_terminate ^ tunnel_0;
    }
    @name(".set_nexthop_details_for_post_routed_flood") action _set_nexthop_details_for_post_routed_flood_0(@name("bd") bit<16> bd_33, @name("uuc_mc_index") bit<16> uuc_mc_index_2) {
        standard_metadata.mcast_grp = uuc_mc_index_2;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd_33;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".ecmp_group") table _ecmp_group {
        actions = {
            _nop_86();
            _set_ecmp_nexthop_details_0();
            _set_ecmp_nexthop_details_for_post_routed_flood_0();
            @defaultonly NoAction_107();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("l3_metadata.nexthop_index");
            meta.hash_metadata.hash1      : selector @name("hash_metadata.hash1");
        }
        size = 1024;
        implementation = ecmp_action_profile;
        default_action = NoAction_107();
    }
    @name(".nexthop") table _nexthop {
        actions = {
            _nop_87();
            _set_nexthop_details_0();
            _set_nexthop_details_for_post_routed_flood_0();
            @defaultonly NoAction_108();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("l3_metadata.nexthop_index");
        }
        size = 1024;
        default_action = NoAction_108();
    }
    @name(".nop") action _nop_88() {
    }
    @name(".set_bd_flood_mc_index") action _set_bd_flood_mc_index_0(@name("mc_index") bit<16> mc_index_42) {
        standard_metadata.mcast_grp = mc_index_42;
    }
    @name(".bd_flood") table _bd_flood {
        actions = {
            _nop_88();
            _set_bd_flood_mc_index_0();
            @defaultonly NoAction_109();
        }
        key = {
            meta.ingress_metadata.bd     : exact @name("ingress_metadata.bd");
            meta.l2_metadata.lkp_pkt_type: exact @name("l2_metadata.lkp_pkt_type");
        }
        size = 1024;
        default_action = NoAction_109();
    }
    @name(".set_lag_miss") action _set_lag_miss_0() {
    }
    @name(".set_lag_port") action _set_lag_port_0(@name("port") bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".set_lag_remote_port") action _set_lag_remote_port_0(@name("device") bit<8> device, @name("port") bit<16> port_3) {
        meta.fabric_metadata.dst_device = device;
        meta.fabric_metadata.dst_port = port_3;
    }
    @name(".lag_group") table _lag_group {
        actions = {
            _set_lag_miss_0();
            _set_lag_port_0();
            _set_lag_remote_port_0();
            @defaultonly NoAction_110();
        }
        key = {
            meta.ingress_metadata.egress_ifindex: exact @name("ingress_metadata.egress_ifindex");
            meta.hash_metadata.hash2            : selector @name("hash_metadata.hash2");
        }
        size = 1024;
        implementation = lag_action_profile;
        default_action = NoAction_110();
    }
    @name(".nop") action _nop_89() {
    }
    @name(".generate_learn_notify") action _generate_learn_notify_0() {
        digest<mac_learn_digest>(32w1024, (mac_learn_digest){bd = meta.ingress_metadata.bd,lkp_mac_sa = meta.l2_metadata.lkp_mac_sa,ifindex = meta.ingress_metadata.ifindex});
    }
    @name(".learn_notify") table _learn_notify {
        actions = {
            _nop_89();
            _generate_learn_notify_0();
            @defaultonly NoAction_111();
        }
        key = {
            meta.l2_metadata.l2_src_miss: ternary @name("l2_metadata.l2_src_miss");
            meta.l2_metadata.l2_src_move: ternary @name("l2_metadata.l2_src_move");
            meta.l2_metadata.stp_state  : ternary @name("l2_metadata.stp_state");
        }
        size = 512;
        default_action = NoAction_111();
    }
    @name(".nop") action _nop_90() {
    }
    @name(".set_fabric_lag_port") action _set_fabric_lag_port_0(@name("port") bit<9> port_4) {
        standard_metadata.egress_spec = port_4;
    }
    @name(".set_fabric_multicast") action _set_fabric_multicast_0(@name("fabric_mgid") bit<8> fabric_mgid_2) {
        meta.multicast_metadata.mcast_grp = standard_metadata.mcast_grp;
    }
    @name(".fabric_lag") table _fabric_lag {
        actions = {
            _nop_90();
            _set_fabric_lag_port_0();
            _set_fabric_multicast_0();
            @defaultonly NoAction_112();
        }
        key = {
            meta.fabric_metadata.dst_device: exact @name("fabric_metadata.dst_device");
            meta.hash_metadata.hash2       : selector @name("hash_metadata.hash2");
        }
        implementation = fabric_lag_action_profile;
        default_action = NoAction_112();
    }
    @name(".drop_stats_update") action _drop_stats_update_0() {
        drop_stats_2.count((bit<10>)meta.ingress_metadata.drop_reason);
    }
    @name(".nop") action _nop_91() {
    }
    @name(".copy_to_cpu_with_reason") action _copy_to_cpu_with_reason_0(@name("reason_code") bit<16> reason_code_6) {
        meta.fabric_metadata.reason_code = reason_code_6;
        clone_preserving_field_list(CloneType.I2E, 32w250, 8w2);
    }
    @name(".redirect_to_cpu") action _redirect_to_cpu_0(@name("reason_code") bit<16> reason_code_7) {
        meta.fabric_metadata.reason_code = reason_code_7;
        clone_preserving_field_list(CloneType.I2E, 32w250, 8w2);
        mark_to_drop(standard_metadata);
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".copy_to_cpu") action _copy_to_cpu_0() {
        clone_preserving_field_list(CloneType.I2E, 32w250, 8w2);
    }
    @name(".drop_packet") action _drop_packet_0() {
        mark_to_drop(standard_metadata);
    }
    @name(".drop_packet_with_reason") action _drop_packet_with_reason_0(@name("drop_reason") bit<10> drop_reason_9) {
        drop_stats.count(drop_reason_9);
        mark_to_drop(standard_metadata);
    }
    @name(".negative_mirror") action _negative_mirror_0(@name("session_id") bit<32> session_id_11) {
        clone_preserving_field_list(CloneType.I2E, session_id_11, 8w5);
        mark_to_drop(standard_metadata);
    }
    @name(".drop_stats") table _drop_stats {
        actions = {
            _drop_stats_update_0();
            @defaultonly NoAction_113();
        }
        size = 1024;
        default_action = NoAction_113();
    }
    @name(".system_acl") table _system_acl {
        actions = {
            _nop_91();
            _redirect_to_cpu_0();
            _copy_to_cpu_with_reason_0();
            _copy_to_cpu_0();
            _drop_packet_0();
            _drop_packet_with_reason_0();
            _negative_mirror_0();
            @defaultonly NoAction_114();
        }
        key = {
            meta.acl_metadata.if_label                : ternary @name("acl_metadata.if_label");
            meta.acl_metadata.bd_label                : ternary @name("acl_metadata.bd_label");
            meta.l2_metadata.lkp_mac_sa               : ternary @name("l2_metadata.lkp_mac_sa");
            meta.l2_metadata.lkp_mac_da               : ternary @name("l2_metadata.lkp_mac_da");
            meta.l2_metadata.lkp_mac_type             : ternary @name("l2_metadata.lkp_mac_type");
            meta.ingress_metadata.ifindex             : ternary @name("ingress_metadata.ifindex");
            meta.l2_metadata.port_vlan_mapping_miss   : ternary @name("l2_metadata.port_vlan_mapping_miss");
            meta.security_metadata.ipsg_check_fail    : ternary @name("security_metadata.ipsg_check_fail");
            meta.security_metadata.storm_control_color: ternary @name("security_metadata.storm_control_color");
            meta.acl_metadata.acl_deny                : ternary @name("acl_metadata.acl_deny");
            meta.acl_metadata.racl_deny               : ternary @name("acl_metadata.racl_deny");
            meta.l3_metadata.urpf_check_fail          : ternary @name("l3_metadata.urpf_check_fail");
            meta.ingress_metadata.drop_flag           : ternary @name("ingress_metadata.drop_flag");
            meta.acl_metadata.acl_copy                : ternary @name("acl_metadata.acl_copy");
            meta.l3_metadata.l3_copy                  : ternary @name("l3_metadata.l3_copy");
            meta.l3_metadata.rmac_hit                 : ternary @name("l3_metadata.rmac_hit");
            meta.l3_metadata.routed                   : ternary @name("l3_metadata.routed");
            meta.ipv6_metadata.ipv6_src_is_link_local : ternary @name("ipv6_metadata.ipv6_src_is_link_local");
            meta.l2_metadata.same_if_check            : ternary @name("l2_metadata.same_if_check");
            meta.tunnel_metadata.tunnel_if_check      : ternary @name("tunnel_metadata.tunnel_if_check");
            meta.l3_metadata.same_bd_check            : ternary @name("l3_metadata.same_bd_check");
            meta.l3_metadata.lkp_ip_ttl               : ternary @name("l3_metadata.lkp_ip_ttl");
            meta.l2_metadata.stp_state                : ternary @name("l2_metadata.stp_state");
            meta.ingress_metadata.control_frame       : ternary @name("ingress_metadata.control_frame");
            meta.ipv4_metadata.ipv4_unicast_enabled   : ternary @name("ipv4_metadata.ipv4_unicast_enabled");
            meta.ipv6_metadata.ipv6_unicast_enabled   : ternary @name("ipv6_metadata.ipv6_unicast_enabled");
            meta.ingress_metadata.egress_ifindex      : ternary @name("ingress_metadata.egress_ifindex");
        }
        size = 512;
        default_action = NoAction_114();
    }
    apply {
        _ingress_port_mapping.apply();
        _ingress_port_properties.apply();
        switch (_validate_outer_ethernet.apply().action_run) {
            _malformed_outer_ethernet_packet_0: {
            }
            default: {
                if (hdr.ipv4.isValid()) {
                    _validate_outer_ipv4_packet_0.apply();
                } else if (hdr.ipv6.isValid()) {
                    _validate_outer_ipv6_packet_0.apply();
                } else if (hdr.mpls[0].isValid()) {
                    _validate_mpls_packet_0.apply();
                }
            }
        }
        _switch_config_params.apply();
        _port_vlan_mapping.apply();
        if (meta.ingress_metadata.port_type == 2w0 && meta.l2_metadata.stp_group != 10w0) {
            _spanning_tree.apply();
        }
        if (meta.ingress_metadata.port_type == 2w0 && meta.security_metadata.ipsg_enabled == 1w1) {
            switch (_ipsg.apply().action_run) {
                _on_miss: {
                    _ipsg_permit_special.apply();
                }
                default: {
                }
            }
        }
        if (hdr.int_header.isValid()) {
            _int_terminate.apply();
            _int_sink_update_outer.apply();
        } else {
            _int_source.apply();
        }
        if (meta.ingress_metadata.port_type != 2w0) {
            _fabric_ingress_dst_lkp_0.apply();
            if (meta.ingress_metadata.port_type == 2w1) {
                if (hdr.fabric_header_multicast.isValid()) {
                    _fabric_ingress_src_lkp_0.apply();
                }
                if (meta.tunnel_metadata.tunnel_terminate == 1w0) {
                    _native_packet_over_fabric_0.apply();
                }
            }
        }
        if (meta.tunnel_metadata.ingress_tunnel_type != 5w0) {
            switch (_outer_rmac.apply().action_run) {
                _on_miss_0: {
                    if (hdr.ipv4.isValid()) {
                        switch (_outer_ipv4_multicast_0.apply().action_run) {
                            _on_miss_1: {
                                _outer_ipv4_multicast_star_g_0.apply();
                            }
                            default: {
                            }
                        }
                    } else if (hdr.ipv6.isValid()) {
                        switch (_outer_ipv6_multicast_0.apply().action_run) {
                            _on_miss_2: {
                                _outer_ipv6_multicast_star_g_0.apply();
                            }
                            default: {
                            }
                        }
                    }
                }
                default: {
                    if (hdr.ipv4.isValid()) {
                        switch (_ipv4_src_vtep_0.apply().action_run) {
                            _src_vtep_hit: {
                                _ipv4_dest_vtep_0.apply();
                            }
                            default: {
                            }
                        }
                    } else if (hdr.ipv6.isValid()) {
                        switch (_ipv6_src_vtep_0.apply().action_run) {
                            _src_vtep_hit_0: {
                                _ipv6_dest_vtep_0.apply();
                            }
                            default: {
                            }
                        }
                    } else if (hdr.mpls[0].isValid()) {
                        _mpls_0.apply();
                    }
                }
            }
        }
        if (meta.tunnel_metadata.tunnel_terminate == 1w1 || meta.multicast_metadata.outer_mcast_route_hit == 1w1 && (meta.multicast_metadata.outer_mcast_mode == 2w1 && meta.multicast_metadata.mcast_rpf_group == 16w0 || meta.multicast_metadata.outer_mcast_mode == 2w2 && meta.multicast_metadata.mcast_rpf_group != 16w0)) {
            switch (_tunnel.apply().action_run) {
                _tunnel_lookup_miss_0: {
                    _tunnel_lookup_miss_1.apply();
                }
                default: {
                }
            }
        } else {
            _tunnel_miss.apply();
        }
        _sflow_ingress.apply();
        _sflow_ing_take_sample.apply();
        if (meta.ingress_metadata.port_type == 2w0) {
            _storm_control.apply();
        }
        if (meta.ingress_metadata.port_type != 2w1) {
            if (hdr.mpls[0].isValid() && meta.l3_metadata.fib_hit == 1w1) {
                ;
            } else {
                if (meta.ingress_metadata.drop_flag == 1w0) {
                    _validate_packet.apply();
                }
                if (meta.ingress_metadata.port_type == 2w0) {
                    _smac.apply();
                }
                if (meta.ingress_metadata.bypass_lookups & 16w0x1 == 16w0) {
                    _dmac.apply();
                }
                if (meta.l3_metadata.lkp_ip_type == 2w0) {
                    if (meta.ingress_metadata.bypass_lookups & 16w0x4 == 16w0) {
                        _mac_acl.apply();
                    }
                } else if (meta.ingress_metadata.bypass_lookups & 16w0x4 == 16w0) {
                    if (meta.l3_metadata.lkp_ip_type == 2w1) {
                        _ip_acl.apply();
                    } else if (meta.l3_metadata.lkp_ip_type == 2w2) {
                        _ipv6_acl.apply();
                    }
                }
                _qos.apply();
                switch (rmac_0.apply().action_run) {
                    rmac_miss: {
                        if (meta.l3_metadata.lkp_ip_type == 2w1) {
                            if (meta.ingress_metadata.bypass_lookups & 16w0x1 == 16w0) {
                                switch (_ipv4_multicast_bridge_0.apply().action_run) {
                                    _on_miss_5: {
                                        _ipv4_multicast_bridge_star_g_0.apply();
                                    }
                                    default: {
                                    }
                                }
                            }
                            if (meta.ingress_metadata.bypass_lookups & 16w0x2 == 16w0 && meta.multicast_metadata.ipv4_multicast_enabled == 1w1) {
                                switch (_ipv4_multicast_route_0.apply().action_run) {
                                    _on_miss_6: {
                                        _ipv4_multicast_route_star_g_0.apply();
                                    }
                                    default: {
                                    }
                                }
                            }
                        } else if (meta.l3_metadata.lkp_ip_type == 2w2) {
                            if (meta.ingress_metadata.bypass_lookups & 16w0x1 == 16w0) {
                                switch (_ipv6_multicast_bridge_0.apply().action_run) {
                                    _on_miss_7: {
                                        _ipv6_multicast_bridge_star_g_0.apply();
                                    }
                                    default: {
                                    }
                                }
                            }
                            if (meta.ingress_metadata.bypass_lookups & 16w0x2 == 16w0 && meta.multicast_metadata.ipv6_multicast_enabled == 1w1) {
                                switch (_ipv6_multicast_route_0.apply().action_run) {
                                    _on_miss_8: {
                                        _ipv6_multicast_route_star_g_0.apply();
                                    }
                                    default: {
                                    }
                                }
                            }
                        }
                    }
                    default: {
                        if (meta.ingress_metadata.bypass_lookups & 16w0x2 == 16w0) {
                            if (meta.l3_metadata.lkp_ip_type == 2w1 && meta.ipv4_metadata.ipv4_unicast_enabled == 1w1) {
                                _ipv4_racl.apply();
                                if (meta.ipv4_metadata.ipv4_urpf_mode != 2w0) {
                                    switch (_ipv4_urpf.apply().action_run) {
                                        _on_miss_23: {
                                            _ipv4_urpf_lpm.apply();
                                        }
                                        default: {
                                        }
                                    }
                                }
                                switch (_ipv4_fib.apply().action_run) {
                                    _on_miss_24: {
                                        _ipv4_fib_lpm.apply();
                                    }
                                    default: {
                                    }
                                }
                            } else if (meta.l3_metadata.lkp_ip_type == 2w2 && meta.ipv6_metadata.ipv6_unicast_enabled == 1w1) {
                                _ipv6_racl.apply();
                                if (meta.ipv6_metadata.ipv6_urpf_mode != 2w0) {
                                    switch (_ipv6_urpf.apply().action_run) {
                                        _on_miss_26: {
                                            _ipv6_urpf_lpm.apply();
                                        }
                                        default: {
                                        }
                                    }
                                }
                                switch (_ipv6_fib.apply().action_run) {
                                    _on_miss_27: {
                                        _ipv6_fib_lpm.apply();
                                    }
                                    default: {
                                    }
                                }
                            }
                            if (meta.l3_metadata.urpf_mode == 2w2 && meta.l3_metadata.urpf_hit == 1w1) {
                                _urpf_bd.apply();
                            }
                        }
                    }
                }
            }
        }
        if (meta.ingress_metadata.bypass_lookups & 16w0x10 == 16w0) {
            _meter_index_0.apply();
        }
        if (meta.tunnel_metadata.tunnel_terminate == 1w0 && hdr.ipv4.isValid() || meta.tunnel_metadata.tunnel_terminate == 1w1 && hdr.inner_ipv4.isValid()) {
            _compute_ipv4_hashes.apply();
        } else if (meta.tunnel_metadata.tunnel_terminate == 1w0 && hdr.ipv6.isValid() || meta.tunnel_metadata.tunnel_terminate == 1w1 && hdr.inner_ipv6.isValid()) {
            _compute_ipv6_hashes.apply();
        } else {
            _compute_non_ip_hashes.apply();
        }
        _compute_other_hashes.apply();
        if (meta.ingress_metadata.bypass_lookups & 16w0x10 == 16w0) {
            _meter_action.apply();
        }
        if (meta.ingress_metadata.port_type != 2w1) {
            _ingress_bd_stats.apply();
            _acl_stats.apply();
            _storm_control_stats_0.apply();
            if (meta.ingress_metadata.bypass_lookups != 16w0xffff) {
                _fwd_result.apply();
            }
            if (meta.nexthop_metadata.nexthop_type == 1w1) {
                _ecmp_group.apply();
            } else {
                _nexthop.apply();
            }
            if (meta.ingress_metadata.egress_ifindex == 16w65535) {
                _bd_flood.apply();
            } else {
                _lag_group.apply();
            }
            if (meta.l2_metadata.learning_enabled == 1w1) {
                _learn_notify.apply();
            }
        }
        _fabric_lag.apply();
        if (meta.ingress_metadata.port_type != 2w1) {
            if (meta.ingress_metadata.bypass_lookups & 16w0x20 == 16w0) {
                _system_acl.apply();
                if (meta.ingress_metadata.drop_flag == 1w1) {
                    _drop_stats.apply();
                }
            }
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<fabric_header_t>(hdr.fabric_header);
        packet.emit<fabric_header_cpu_t>(hdr.fabric_header_cpu);
        packet.emit<fabric_header_sflow_t>(hdr.fabric_header_sflow);
        packet.emit<fabric_header_mirror_t>(hdr.fabric_header_mirror);
        packet.emit<fabric_header_multicast_t>(hdr.fabric_header_multicast);
        packet.emit<fabric_header_unicast_t>(hdr.fabric_header_unicast);
        packet.emit<fabric_payload_header_t>(hdr.fabric_payload_header);
        packet.emit<llc_header_t>(hdr.llc_header);
        packet.emit<snap_header_t>(hdr.snap_header);
        packet.emit<vlan_tag_t>(hdr.vlan_tag_[0]);
        packet.emit<vlan_tag_t>(hdr.vlan_tag_[1]);
        packet.emit<arp_rarp_t>(hdr.arp_rarp);
        packet.emit<arp_rarp_ipv4_t>(hdr.arp_rarp_ipv4);
        packet.emit<ipv6_t>(hdr.ipv6);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<gre_t>(hdr.gre);
        packet.emit<erspan_header_t3_t_0>(hdr.erspan_t3_header);
        packet.emit<nvgre_t>(hdr.nvgre);
        packet.emit<udp_t>(hdr.udp);
        packet.emit<sflow_hdr_t>(hdr.sflow);
        packet.emit<vxlan_gpe_t>(hdr.vxlan_gpe);
        packet.emit<vxlan_gpe_int_header_t>(hdr.vxlan_gpe_int_header);
        packet.emit<int_header_t>(hdr.int_header);
        packet.emit<int_switch_id_header_t>(hdr.int_switch_id_header);
        packet.emit<int_ingress_port_id_header_t>(hdr.int_ingress_port_id_header);
        packet.emit<int_hop_latency_header_t>(hdr.int_hop_latency_header);
        packet.emit<int_q_occupancy_header_t>(hdr.int_q_occupancy_header);
        packet.emit<int_ingress_tstamp_header_t>(hdr.int_ingress_tstamp_header);
        packet.emit<int_egress_port_id_header_t>(hdr.int_egress_port_id_header);
        packet.emit<int_q_congestion_header_t>(hdr.int_q_congestion_header);
        packet.emit<int_egress_port_tx_utilization_header_t>(hdr.int_egress_port_tx_utilization_header);
        packet.emit<int_value_t[24]>(hdr.int_val);
        packet.emit<genv_t>(hdr.genv);
        packet.emit<vxlan_t>(hdr.vxlan);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<icmp_t>(hdr.icmp);
        packet.emit<mpls_t[3]>(hdr.mpls);
        packet.emit<ethernet_t>(hdr.inner_ethernet);
        packet.emit<ipv6_t>(hdr.inner_ipv6);
        packet.emit<ipv4_t>(hdr.inner_ipv4);
        packet.emit<udp_t>(hdr.inner_udp);
        packet.emit<tcp_t>(hdr.inner_tcp);
        packet.emit<icmp_t>(hdr.inner_icmp);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.inner_ipv4.ihl == 4w5, { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr }, hdr.inner_ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.inner_ipv4.ihl == 4w5, { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr }, hdr.inner_ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
