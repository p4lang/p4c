#include <core.p4>
#include <v1model.p4>

struct acl_metadata_t {
    bit<1>  acl_deny;
    bit<1>  racl_deny;
    bit<16> acl_nexthop;
    bit<16> racl_nexthop;
    bit<1>  acl_nexthop_type;
    bit<1>  racl_nexthop_type;
    bit<1>  acl_redirect;
    bit<1>  racl_redirect;
    bit<15> if_label;
    bit<16> bd_label;
    bit<10> mirror_session_id;
    bit<16> acl_stats_index;
}

struct egress_filter_metadata_t {
    bit<16> ifindex;
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
}

struct fabric_metadata_t {
    bit<3>  packetType;
    bit<1>  fabric_header_present;
    bit<16> reason_code;
    bit<8>  dst_device;
    bit<16> dst_port;
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
    bit<1>  enable_dod;
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
    bit<1>  resubmit_flag;
    bit<48> ingress_global_tstamp;
    bit<16> mcast_grp;
    bit<1>  deflection_flag;
    bit<1>  deflect_on_drop;
    bit<19> enq_qdepth;
    bit<32> enq_tstamp;
    bit<2>  enq_congest_stat;
    bit<19> deq_qdepth;
    bit<2>  deq_congest_stat;
    bit<32> deq_timedelta;
    bit<13> mcast_hash;
    bit<16> egress_rid;
    bit<32> lf_field_list;
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
    bit<3>  lkp_pkt_type;
    bit<48> lkp_mac_sa;
    bit<48> lkp_mac_da;
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
    bit<16> lkp_inner_l4_sport;
    bit<16> lkp_inner_l4_dport;
    bit<12> vrf;
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
    bit<16> l3_mtu_check;
}

struct multicast_metadata_t {
    bit<1>  ip_multicast;
    bit<1>  igmp_snooping_enabled;
    bit<1>  mld_snooping_enabled;
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

struct security_metadata_t {
    bit<1> storm_control_color;
    bit<1> ipsg_enabled;
    bit<1> ipsg_check_fail;
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
    bit<31> ingress_port_id;
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
    bit<31> q_occupancy;
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

header sflow_t {
    bit<32> version;
    bit<32> ipVersion;
    bit<32> ipAddress;
    bit<32> subAgentId;
    bit<32> seqNumber;
    bit<32> uptime;
    bit<32> numSamples;
}

header sflow_internal_ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header sflow_record_t {
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
    bit<8>  srcIdClass;
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
    acl_metadata_t               acl_metadata;
    @name(".egress_filter_metadata") 
    egress_filter_metadata_t     egress_filter_metadata;
    @name(".egress_metadata") 
    egress_metadata_t            egress_metadata;
    @name(".fabric_metadata") 
    fabric_metadata_t            fabric_metadata;
    @pa_atomic("ingress", "hash_metadata.hash1") @pa_solitary("ingress", "hash_metadata.hash1") @pa_atomic("ingress", "hash_metadata.hash2") @pa_solitary("ingress", "hash_metadata.hash2") @name(".hash_metadata") 
    hash_metadata_t              hash_metadata;
    @name(".i2e_metadata") 
    i2e_metadata_t               i2e_metadata;
    @name(".ingress_metadata") 
    ingress_metadata_t           ingress_metadata;
    @name(".int_metadata") 
    int_metadata_t               int_metadata;
    @name(".int_metadata_i2e") 
    int_metadata_i2e_t           int_metadata_i2e;
    @name(".intrinsic_metadata") 
    ingress_intrinsic_metadata_t intrinsic_metadata;
    @name(".ipv4_metadata") 
    ipv4_metadata_t              ipv4_metadata;
    @name(".ipv6_metadata") 
    ipv6_metadata_t              ipv6_metadata;
    @name(".l2_metadata") 
    l2_metadata_t                l2_metadata;
    @name(".l3_metadata") 
    l3_metadata_t                l3_metadata;
    @name(".multicast_metadata") 
    multicast_metadata_t         multicast_metadata;
    @name(".nexthop_metadata") 
    nexthop_metadata_t           nexthop_metadata;
    @name(".qos_metadata") 
    qos_metadata_t               qos_metadata;
    @name(".security_metadata") 
    security_metadata_t          security_metadata;
    @name(".tunnel_metadata") 
    tunnel_metadata_t            tunnel_metadata;
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
    sflow_t                                 sflow;
    @name(".sflow_internal_ethernet") 
    sflow_internal_ethernet_t               sflow_internal_ethernet;
    @name(".sflow_record") 
    sflow_record_t                          sflow_record;
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
    @name(".mpls") 
    mpls_t[3]                               mpls;
    @name(".vlan_tag_") 
    vlan_tag_t[2]                           vlan_tag_;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<4> tmp_0;
    @name(".parse_all_int_meta_value_heders") state parse_all_int_meta_value_heders {
        packet.extract<int_switch_id_header_t>(hdr.int_switch_id_header);
        packet.extract<int_ingress_port_id_header_t>(hdr.int_ingress_port_id_header);
        packet.extract<int_hop_latency_header_t>(hdr.int_hop_latency_header);
        packet.extract<int_q_occupancy_header_t>(hdr.int_q_occupancy_header);
        packet.extract<int_ingress_tstamp_header_t>(hdr.int_ingress_tstamp_header);
        packet.extract<int_egress_port_id_header_t>(hdr.int_egress_port_id_header);
        packet.extract<int_q_congestion_header_t>(hdr.int_q_congestion_header);
        packet.extract<int_egress_port_tx_utilization_header_t>(hdr.int_egress_port_tx_utilization_header);
        transition accept;
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
        meta.l2_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
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
        transition parse_fabric_payload_header;
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
        meta.l3_metadata.lkp_l4_sport = hdr.icmp.typeCode;
        transition select(hdr.icmp.typeCode) {
            16w0x8200 &&& 16w0xfe00: parse_set_prio_med;
            16w0x8400 &&& 16w0xfc00: parse_set_prio_med;
            16w0x8800 &&& 16w0xff00: parse_set_prio_med;
            default: accept;
        }
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
        meta.l3_metadata.lkp_inner_l4_sport = hdr.inner_icmp.typeCode;
        transition accept;
    }
    @name(".parse_inner_ipv4") state parse_inner_ipv4 {
        packet.extract<ipv4_t>(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ihl, hdr.inner_ipv4.protocol) {
            (13w0x0, 4w0x5, 8w0x1): parse_inner_icmp;
            (13w0x0, 4w0x5, 8w0x6): parse_inner_tcp;
            (13w0x0, 4w0x5, 8w0x11): parse_inner_udp;
            default: accept;
        }
    }
    @name(".parse_inner_ipv6") state parse_inner_ipv6 {
        packet.extract<ipv6_t>(hdr.inner_ipv6);
        transition select(hdr.inner_ipv6.nextHdr) {
            8w58: parse_inner_icmp;
            8w6: parse_inner_tcp;
            8w17: parse_inner_udp;
            default: accept;
        }
    }
    @name(".parse_inner_tcp") state parse_inner_tcp {
        packet.extract<tcp_t>(hdr.inner_tcp);
        meta.l3_metadata.lkp_inner_l4_sport = hdr.inner_tcp.srcPort;
        meta.l3_metadata.lkp_inner_l4_dport = hdr.inner_tcp.dstPort;
        transition accept;
    }
    @name(".parse_inner_udp") state parse_inner_udp {
        packet.extract<udp_t>(hdr.inner_udp);
        meta.l3_metadata.lkp_inner_l4_sport = hdr.inner_udp.srcPort;
        meta.l3_metadata.lkp_inner_l4_dport = hdr.inner_udp.dstPort;
        transition accept;
    }
    @name(".parse_int_header") state parse_int_header {
        packet.extract<int_header_t>(hdr.int_header);
        meta.int_metadata.instruction_cnt = (bit<16>)hdr.int_header.ins_cnt;
        transition select(hdr.int_header.rsvd1, hdr.int_header.total_hop_cnt) {
            (5w0x0, 8w0x0): accept;
            (5w0x1, 8w0x0): parse_all_int_meta_value_heders;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.ipv4.protocol;
        meta.l3_metadata.lkp_ip_ttl = hdr.ipv4.ttl;
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
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.ipv6.nextHdr;
        meta.l3_metadata.lkp_ip_ttl = hdr.ipv6.hopLimit;
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
        transition select(tmp_0[3:0]) {
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
        meta.intrinsic_metadata.priority = 3w5;
        transition accept;
    }
    @name(".parse_set_prio_med") state parse_set_prio_med {
        meta.intrinsic_metadata.priority = 3w3;
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
        meta.l3_metadata.lkp_l4_sport = hdr.tcp.srcPort;
        meta.l3_metadata.lkp_l4_dport = hdr.tcp.dstPort;
        transition select(hdr.tcp.dstPort) {
            16w179: parse_set_prio_med;
            16w639: parse_set_prio_med;
            default: accept;
        }
    }
    @name(".parse_udp") state parse_udp {
        packet.extract<udp_t>(hdr.udp);
        meta.l3_metadata.lkp_l4_sport = hdr.udp.srcPort;
        meta.l3_metadata.lkp_l4_dport = hdr.udp.dstPort;
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

struct tuple_0 {
    bit<32> field;
    bit<16> field_0;
}

struct tuple_1 {
    bit<16> field_1;
    bit<16> field_2;
    bit<16> field_3;
    bit<9>  field_4;
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name("NoAction") action NoAction_1() {
    }
    @name("NoAction") action NoAction_86() {
    }
    @name("NoAction") action NoAction_87() {
    }
    @name("NoAction") action NoAction_88() {
    }
    @name("NoAction") action NoAction_89() {
    }
    @name("NoAction") action NoAction_90() {
    }
    @name("NoAction") action NoAction_91() {
    }
    @name("NoAction") action NoAction_92() {
    }
    @name("NoAction") action NoAction_93() {
    }
    @name("NoAction") action NoAction_94() {
    }
    @name("NoAction") action NoAction_95() {
    }
    @name("NoAction") action NoAction_96() {
    }
    @name("NoAction") action NoAction_97() {
    }
    @name("NoAction") action NoAction_98() {
    }
    @name("NoAction") action NoAction_99() {
    }
    @name("NoAction") action NoAction_100() {
    }
    @name("NoAction") action NoAction_101() {
    }
    @name("NoAction") action NoAction_102() {
    }
    @name("NoAction") action NoAction_103() {
    }
    @name("NoAction") action NoAction_104() {
    }
    @name("NoAction") action NoAction_105() {
    }
    @name("NoAction") action NoAction_106() {
    }
    @name("NoAction") action NoAction_107() {
    }
    @name("NoAction") action NoAction_108() {
    }
    @name("NoAction") action NoAction_109() {
    }
    @name("NoAction") action NoAction_110() {
    }
    @name("NoAction") action NoAction_111() {
    }
    @name("NoAction") action NoAction_112() {
    }
    @name("NoAction") action NoAction_113() {
    }
    @name(".egress_port_type_normal") action egress_port_type_normal_0() {
        meta.egress_metadata.port_type = 2w0;
    }
    @name(".egress_port_type_fabric") action egress_port_type_fabric_0() {
        meta.egress_metadata.port_type = 2w1;
        meta.tunnel_metadata.egress_tunnel_type = 5w15;
    }
    @name(".egress_port_type_cpu") action egress_port_type_cpu_0() {
        meta.egress_metadata.port_type = 2w2;
        meta.tunnel_metadata.egress_tunnel_type = 5w16;
    }
    @name(".nop") action nop_0() {
    }
    @name(".set_mirror_nhop") action set_mirror_nhop_0(bit<16> nhop_idx) {
        meta.l3_metadata.nexthop_index = nhop_idx;
    }
    @name(".set_mirror_bd") action set_mirror_bd_0(bit<16> bd) {
        meta.egress_metadata.bd = bd;
    }
    @name(".egress_port_mapping") table egress_port_mapping {
        actions = {
            egress_port_type_normal_0();
            egress_port_type_fabric_0();
            egress_port_type_cpu_0();
            @defaultonly NoAction_0();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
        }
        size = 288;
        default_action = NoAction_0();
    }
    @name(".mirror") table mirror {
        actions = {
            nop_0();
            set_mirror_nhop_0();
            set_mirror_bd_0();
            @defaultonly NoAction_1();
        }
        key = {
            meta.i2e_metadata.mirror_session_id: exact @name("i2e_metadata.mirror_session_id") ;
        }
        size = 1024;
        default_action = NoAction_1();
    }
    @name(".nop") action _nop_1() {
    }
    @name(".nop") action _nop_2() {
    }
    @name(".set_replica_copy_bridged") action _set_replica_copy_bridged() {
        meta.egress_metadata.routed = 1w0;
    }
    @name(".outer_replica_from_rid") action _outer_replica_from_rid(bit<16> bd, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.bd = bd;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w0;
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.same_bd_check = bd ^ meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".outer_replica_from_rid_with_nexthop") action _outer_replica_from_rid_with_nexthop(bit<16> bd, bit<16> nexthop_index, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.bd = bd;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w0;
        meta.egress_metadata.routed = meta.l3_metadata.outer_routed;
        meta.l3_metadata.nexthop_index = nexthop_index;
        meta.egress_metadata.same_bd_check = bd ^ meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".inner_replica_from_rid") action _inner_replica_from_rid(bit<16> bd, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.bd = bd;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w1;
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.same_bd_check = bd ^ meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".inner_replica_from_rid_with_nexthop") action _inner_replica_from_rid_with_nexthop(bit<16> bd, bit<16> nexthop_index, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.bd = bd;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w1;
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.l3_metadata.nexthop_index = nexthop_index;
        meta.egress_metadata.same_bd_check = bd ^ meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".replica_type") table _replica_type_0 {
        actions = {
            _nop_1();
            _set_replica_copy_bridged();
            @defaultonly NoAction_86();
        }
        key = {
            meta.multicast_metadata.replica   : exact @name("multicast_metadata.replica") ;
            meta.egress_metadata.same_bd_check: ternary @name("egress_metadata.same_bd_check") ;
        }
        size = 512;
        default_action = NoAction_86();
    }
    @name(".rid") table _rid_0 {
        actions = {
            _nop_2();
            _outer_replica_from_rid();
            _outer_replica_from_rid_with_nexthop();
            _inner_replica_from_rid();
            _inner_replica_from_rid_with_nexthop();
            @defaultonly NoAction_87();
        }
        key = {
            meta.intrinsic_metadata.egress_rid: exact @name("intrinsic_metadata.egress_rid") ;
        }
        size = 1024;
        default_action = NoAction_87();
    }
    @name(".nop") action _nop_3() {
    }
    @name(".remove_vlan_single_tagged") action _remove_vlan_single_tagged() {
        hdr.ethernet.etherType = hdr.vlan_tag_[0].etherType;
        hdr.vlan_tag_[0].setInvalid();
    }
    @name(".remove_vlan_double_tagged") action _remove_vlan_double_tagged() {
        hdr.ethernet.etherType = hdr.vlan_tag_[1].etherType;
        hdr.vlan_tag_[0].setInvalid();
        hdr.vlan_tag_[1].setInvalid();
    }
    @name(".vlan_decap") table _vlan_decap_0 {
        actions = {
            _nop_3();
            _remove_vlan_single_tagged();
            _remove_vlan_double_tagged();
            @defaultonly NoAction_88();
        }
        key = {
            hdr.vlan_tag_[0].isValid(): exact @name("vlan_tag_[0].$valid$") ;
            hdr.vlan_tag_[1].isValid(): exact @name("vlan_tag_[1].$valid$") ;
        }
        size = 1024;
        default_action = NoAction_88();
    }
    @name(".decap_inner_udp") action _decap_inner_udp() {
        hdr.udp = hdr.inner_udp;
        hdr.inner_udp.setInvalid();
    }
    @name(".decap_inner_tcp") action _decap_inner_tcp() {
        hdr.tcp = hdr.inner_tcp;
        hdr.inner_tcp.setInvalid();
        hdr.udp.setInvalid();
    }
    @name(".decap_inner_icmp") action _decap_inner_icmp() {
        hdr.icmp = hdr.inner_icmp;
        hdr.inner_icmp.setInvalid();
        hdr.udp.setInvalid();
    }
    @name(".decap_inner_unknown") action _decap_inner_unknown() {
        hdr.udp.setInvalid();
    }
    @name(".decap_vxlan_inner_ipv4") action _decap_vxlan_inner_ipv4() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.vxlan.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_vxlan_inner_ipv6") action _decap_vxlan_inner_ipv6() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.vxlan.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_vxlan_inner_non_ip") action _decap_vxlan_inner_non_ip() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.vxlan.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_genv_inner_ipv4") action _decap_genv_inner_ipv4() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.genv.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_genv_inner_ipv6") action _decap_genv_inner_ipv6() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.genv.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_genv_inner_non_ip") action _decap_genv_inner_non_ip() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.genv.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_nvgre_inner_ipv4") action _decap_nvgre_inner_ipv4() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_nvgre_inner_ipv6") action _decap_nvgre_inner_ipv6() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_nvgre_inner_non_ip") action _decap_nvgre_inner_non_ip() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_ip_inner_ipv4") action _decap_ip_inner_ipv4() {
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.gre.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_ip_inner_ipv6") action _decap_ip_inner_ipv6() {
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ipv4_pop1") action _decap_mpls_inner_ipv4_pop1() {
        hdr.mpls[0].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop1") action _decap_mpls_inner_ipv6_pop1() {
        hdr.mpls[0].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop1") action _decap_mpls_inner_ethernet_ipv4_pop1() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop1") action _decap_mpls_inner_ethernet_ipv6_pop1() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop1") action _decap_mpls_inner_ethernet_non_ip_pop1() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".decap_mpls_inner_ipv4_pop2") action _decap_mpls_inner_ipv4_pop2() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop2") action _decap_mpls_inner_ipv6_pop2() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop2") action _decap_mpls_inner_ethernet_ipv4_pop2() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop2") action _decap_mpls_inner_ethernet_ipv6_pop2() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop2") action _decap_mpls_inner_ethernet_non_ip_pop2() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".decap_mpls_inner_ipv4_pop3") action _decap_mpls_inner_ipv4_pop3() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop3") action _decap_mpls_inner_ipv6_pop3() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop3") action _decap_mpls_inner_ethernet_ipv4_pop3() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop3") action _decap_mpls_inner_ethernet_ipv6_pop3() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop3") action _decap_mpls_inner_ethernet_non_ip_pop3() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".tunnel_decap_process_inner") table _tunnel_decap_process_inner_0 {
        actions = {
            _decap_inner_udp();
            _decap_inner_tcp();
            _decap_inner_icmp();
            _decap_inner_unknown();
            @defaultonly NoAction_89();
        }
        key = {
            hdr.inner_tcp.isValid() : exact @name("inner_tcp.$valid$") ;
            hdr.inner_udp.isValid() : exact @name("inner_udp.$valid$") ;
            hdr.inner_icmp.isValid(): exact @name("inner_icmp.$valid$") ;
        }
        size = 1024;
        default_action = NoAction_89();
    }
    @name(".tunnel_decap_process_outer") table _tunnel_decap_process_outer_0 {
        actions = {
            _decap_vxlan_inner_ipv4();
            _decap_vxlan_inner_ipv6();
            _decap_vxlan_inner_non_ip();
            _decap_genv_inner_ipv4();
            _decap_genv_inner_ipv6();
            _decap_genv_inner_non_ip();
            _decap_nvgre_inner_ipv4();
            _decap_nvgre_inner_ipv6();
            _decap_nvgre_inner_non_ip();
            _decap_ip_inner_ipv4();
            _decap_ip_inner_ipv6();
            _decap_mpls_inner_ipv4_pop1();
            _decap_mpls_inner_ipv6_pop1();
            _decap_mpls_inner_ethernet_ipv4_pop1();
            _decap_mpls_inner_ethernet_ipv6_pop1();
            _decap_mpls_inner_ethernet_non_ip_pop1();
            _decap_mpls_inner_ipv4_pop2();
            _decap_mpls_inner_ipv6_pop2();
            _decap_mpls_inner_ethernet_ipv4_pop2();
            _decap_mpls_inner_ethernet_ipv6_pop2();
            _decap_mpls_inner_ethernet_non_ip_pop2();
            _decap_mpls_inner_ipv4_pop3();
            _decap_mpls_inner_ipv6_pop3();
            _decap_mpls_inner_ethernet_ipv4_pop3();
            _decap_mpls_inner_ethernet_ipv6_pop3();
            _decap_mpls_inner_ethernet_non_ip_pop3();
            @defaultonly NoAction_90();
        }
        key = {
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type") ;
            hdr.inner_ipv4.isValid()                : exact @name("inner_ipv4.$valid$") ;
            hdr.inner_ipv6.isValid()                : exact @name("inner_ipv6.$valid$") ;
        }
        size = 1024;
        default_action = NoAction_90();
    }
    @name(".nop") action _nop_4() {
    }
    @name(".set_egress_bd_properties") action _set_egress_bd_properties() {
    }
    @name(".egress_bd_map") table _egress_bd_map_0 {
        actions = {
            _nop_4();
            _set_egress_bd_properties();
            @defaultonly NoAction_91();
        }
        key = {
            meta.egress_metadata.bd: exact @name("egress_metadata.bd") ;
        }
        size = 1024;
        default_action = NoAction_91();
    }
    @name(".nop") action _nop_5() {
    }
    @name(".set_l2_rewrite") action _set_l2_rewrite() {
        meta.egress_metadata.routed = 1w0;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.egress_metadata.outer_bd = meta.ingress_metadata.bd;
    }
    @name(".set_l2_rewrite_with_tunnel") action _set_l2_rewrite_with_tunnel(bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.routed = 1w0;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.egress_metadata.outer_bd = meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".set_l3_rewrite") action _set_l3_rewrite(bit<16> bd, bit<8> mtu_index, bit<9> smac_idx, bit<48> dmac) {
        meta.egress_metadata.routed = 1w1;
        meta.egress_metadata.smac_idx = smac_idx;
        meta.egress_metadata.mac_da = dmac;
        meta.egress_metadata.bd = bd;
        meta.egress_metadata.outer_bd = bd;
        meta.l3_metadata.mtu_index = mtu_index;
    }
    @name(".set_l3_rewrite_with_tunnel") action _set_l3_rewrite_with_tunnel(bit<16> bd, bit<9> smac_idx, bit<48> dmac, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.routed = 1w1;
        meta.egress_metadata.smac_idx = smac_idx;
        meta.egress_metadata.mac_da = dmac;
        meta.egress_metadata.bd = bd;
        meta.egress_metadata.outer_bd = bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".set_mpls_swap_push_rewrite_l2") action _set_mpls_swap_push_rewrite_l2(bit<20> label, bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        hdr.mpls[0].label = label;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w13;
    }
    @name(".set_mpls_push_rewrite_l2") action _set_mpls_push_rewrite_l2(bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w13;
    }
    @name(".set_mpls_swap_push_rewrite_l3") action _set_mpls_swap_push_rewrite_l3(bit<16> bd, bit<9> smac_idx, bit<48> dmac, bit<20> label, bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = bd;
        hdr.mpls[0].label = label;
        meta.egress_metadata.smac_idx = smac_idx;
        meta.egress_metadata.mac_da = dmac;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w14;
    }
    @name(".set_mpls_push_rewrite_l3") action _set_mpls_push_rewrite_l3(bit<16> bd, bit<9> smac_idx, bit<48> dmac, bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = bd;
        meta.egress_metadata.smac_idx = smac_idx;
        meta.egress_metadata.mac_da = dmac;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w14;
    }
    @name(".rewrite") table _rewrite_0 {
        actions = {
            _nop_5();
            _set_l2_rewrite();
            _set_l2_rewrite_with_tunnel();
            _set_l3_rewrite();
            _set_l3_rewrite_with_tunnel();
            _set_mpls_swap_push_rewrite_l2();
            _set_mpls_push_rewrite_l2();
            _set_mpls_swap_push_rewrite_l3();
            _set_mpls_push_rewrite_l3();
            @defaultonly NoAction_92();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("l3_metadata.nexthop_index") ;
        }
        size = 1024;
        default_action = NoAction_92();
    }
    @name(".int_set_header_0_bos") action _int_set_header_0_bos() {
        hdr.int_switch_id_header.bos = 1w1;
    }
    @name(".int_set_header_1_bos") action _int_set_header_1_bos() {
        hdr.int_ingress_port_id_header.bos = 1w1;
    }
    @name(".int_set_header_2_bos") action _int_set_header_2_bos() {
        hdr.int_hop_latency_header.bos = 1w1;
    }
    @name(".int_set_header_3_bos") action _int_set_header_3_bos() {
        hdr.int_q_occupancy_header.bos = 1w1;
    }
    @name(".int_set_header_4_bos") action _int_set_header_4_bos() {
        hdr.int_ingress_tstamp_header.bos = 1w1;
    }
    @name(".int_set_header_5_bos") action _int_set_header_5_bos() {
        hdr.int_egress_port_id_header.bos = 1w1;
    }
    @name(".int_set_header_6_bos") action _int_set_header_6_bos() {
        hdr.int_q_congestion_header.bos = 1w1;
    }
    @name(".int_set_header_7_bos") action _int_set_header_7_bos() {
        hdr.int_egress_port_tx_utilization_header.bos = 1w1;
    }
    @name(".nop") action _nop_6() {
    }
    @name(".nop") action _nop_7() {
    }
    @name(".nop") action _nop_8() {
    }
    @name(".int_transit") action _int_transit(bit<32> switch_id) {
        meta.int_metadata.insert_cnt = hdr.int_header.max_hop_cnt - hdr.int_header.total_hop_cnt;
        meta.int_metadata.switch_id = switch_id;
        meta.int_metadata.insert_byte_cnt = meta.int_metadata.instruction_cnt << 2;
        meta.int_metadata.gpe_int_hdr_len8 = (bit<8>)hdr.int_header.ins_cnt;
    }
    @name(".int_reset") action _int_reset() {
        meta.int_metadata.switch_id = 32w0;
        meta.int_metadata.insert_byte_cnt = 16w0;
        meta.int_metadata.insert_cnt = 8w0;
        meta.int_metadata.gpe_int_hdr_len8 = 8w0;
        meta.int_metadata.gpe_int_hdr_len = 16w0;
        meta.int_metadata.instruction_cnt = 16w0;
    }
    @name(".int_set_header_0003_i0") action _int_set_header_0003_i0() {
    }
    @name(".int_set_header_0003_i1") action _int_set_header_0003_i1() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy = (bit<31>)meta.intrinsic_metadata.enq_qdepth;
    }
    @name(".int_set_header_0003_i2") action _int_set_header_0003_i2() {
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)meta.intrinsic_metadata.deq_timedelta;
    }
    @name(".int_set_header_0003_i3") action _int_set_header_0003_i3() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy = (bit<31>)meta.intrinsic_metadata.enq_qdepth;
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)meta.intrinsic_metadata.deq_timedelta;
    }
    @name(".int_set_header_0003_i4") action _int_set_header_0003_i4() {
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id = (bit<31>)meta.ingress_metadata.ifindex;
    }
    @name(".int_set_header_0003_i5") action _int_set_header_0003_i5() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy = (bit<31>)meta.intrinsic_metadata.enq_qdepth;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id = (bit<31>)meta.ingress_metadata.ifindex;
    }
    @name(".int_set_header_0003_i6") action _int_set_header_0003_i6() {
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)meta.intrinsic_metadata.deq_timedelta;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id = (bit<31>)meta.ingress_metadata.ifindex;
    }
    @name(".int_set_header_0003_i7") action _int_set_header_0003_i7() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy = (bit<31>)meta.intrinsic_metadata.enq_qdepth;
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)meta.intrinsic_metadata.deq_timedelta;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id = (bit<31>)meta.ingress_metadata.ifindex;
    }
    @name(".int_set_header_0003_i8") action _int_set_header_0003_i8() {
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i9") action _int_set_header_0003_i9() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy = (bit<31>)meta.intrinsic_metadata.enq_qdepth;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i10") action _int_set_header_0003_i10() {
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)meta.intrinsic_metadata.deq_timedelta;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i11") action _int_set_header_0003_i11() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy = (bit<31>)meta.intrinsic_metadata.enq_qdepth;
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)meta.intrinsic_metadata.deq_timedelta;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i12") action _int_set_header_0003_i12() {
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id = (bit<31>)meta.ingress_metadata.ifindex;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i13") action _int_set_header_0003_i13() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy = (bit<31>)meta.intrinsic_metadata.enq_qdepth;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id = (bit<31>)meta.ingress_metadata.ifindex;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i14") action _int_set_header_0003_i14() {
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)meta.intrinsic_metadata.deq_timedelta;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id = (bit<31>)meta.ingress_metadata.ifindex;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i15") action _int_set_header_0003_i15() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy = (bit<31>)meta.intrinsic_metadata.enq_qdepth;
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)meta.intrinsic_metadata.deq_timedelta;
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id = (bit<31>)meta.ingress_metadata.ifindex;
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0407_i0") action _int_set_header_0407_i0() {
    }
    @name(".int_set_header_0407_i1") action _int_set_header_0407_i1() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
    }
    @name(".int_set_header_0407_i2") action _int_set_header_0407_i2() {
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
    }
    @name(".int_set_header_0407_i3") action _int_set_header_0407_i3() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
    }
    @name(".int_set_header_0407_i4") action _int_set_header_0407_i4() {
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
    }
    @name(".int_set_header_0407_i5") action _int_set_header_0407_i5() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
    }
    @name(".int_set_header_0407_i6") action _int_set_header_0407_i6() {
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
    }
    @name(".int_set_header_0407_i7") action _int_set_header_0407_i7() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
    }
    @name(".int_set_header_0407_i8") action _int_set_header_0407_i8() {
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i9") action _int_set_header_0407_i9() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i10") action _int_set_header_0407_i10() {
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i11") action _int_set_header_0407_i11() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i12") action _int_set_header_0407_i12() {
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i13") action _int_set_header_0407_i13() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i14") action _int_set_header_0407_i14() {
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i15") action _int_set_header_0407_i15() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_e_bit") action _int_set_e_bit() {
        hdr.int_header.e = 1w1;
    }
    @name(".int_update_total_hop_cnt") action _int_update_total_hop_cnt() {
        hdr.int_header.total_hop_cnt = hdr.int_header.total_hop_cnt + 8w1;
    }
    @name(".int_bos") table _int_bos_0 {
        actions = {
            _int_set_header_0_bos();
            _int_set_header_1_bos();
            _int_set_header_2_bos();
            _int_set_header_3_bos();
            _int_set_header_4_bos();
            _int_set_header_5_bos();
            _int_set_header_6_bos();
            _int_set_header_7_bos();
            _nop_6();
            @defaultonly NoAction_93();
        }
        key = {
            hdr.int_header.total_hop_cnt        : ternary @name("int_header.total_hop_cnt") ;
            hdr.int_header.instruction_mask_0003: ternary @name("int_header.instruction_mask_0003") ;
            hdr.int_header.instruction_mask_0407: ternary @name("int_header.instruction_mask_0407") ;
            hdr.int_header.instruction_mask_0811: ternary @name("int_header.instruction_mask_0811") ;
            hdr.int_header.instruction_mask_1215: ternary @name("int_header.instruction_mask_1215") ;
        }
        size = 16;
        default_action = NoAction_93();
    }
    @name(".int_insert") table _int_insert_0 {
        actions = {
            _int_transit();
            _int_reset();
            @defaultonly NoAction_94();
        }
        key = {
            meta.int_metadata_i2e.source: ternary @name("int_metadata_i2e.source") ;
            meta.int_metadata_i2e.sink  : ternary @name("int_metadata_i2e.sink") ;
            hdr.int_header.isValid()    : exact @name("int_header.$valid$") ;
        }
        size = 2;
        default_action = NoAction_94();
    }
    @name(".int_inst_0003") table _int_inst_3 {
        actions = {
            _int_set_header_0003_i0();
            _int_set_header_0003_i1();
            _int_set_header_0003_i2();
            _int_set_header_0003_i3();
            _int_set_header_0003_i4();
            _int_set_header_0003_i5();
            _int_set_header_0003_i6();
            _int_set_header_0003_i7();
            _int_set_header_0003_i8();
            _int_set_header_0003_i9();
            _int_set_header_0003_i10();
            _int_set_header_0003_i11();
            _int_set_header_0003_i12();
            _int_set_header_0003_i13();
            _int_set_header_0003_i14();
            _int_set_header_0003_i15();
            @defaultonly NoAction_95();
        }
        key = {
            hdr.int_header.instruction_mask_0003: exact @name("int_header.instruction_mask_0003") ;
        }
        size = 16;
        default_action = NoAction_95();
    }
    @name(".int_inst_0407") table _int_inst_4 {
        actions = {
            _int_set_header_0407_i0();
            _int_set_header_0407_i1();
            _int_set_header_0407_i2();
            _int_set_header_0407_i3();
            _int_set_header_0407_i4();
            _int_set_header_0407_i5();
            _int_set_header_0407_i6();
            _int_set_header_0407_i7();
            _int_set_header_0407_i8();
            _int_set_header_0407_i9();
            _int_set_header_0407_i10();
            _int_set_header_0407_i11();
            _int_set_header_0407_i12();
            _int_set_header_0407_i13();
            _int_set_header_0407_i14();
            _int_set_header_0407_i15();
            @defaultonly NoAction_96();
        }
        key = {
            hdr.int_header.instruction_mask_0407: exact @name("int_header.instruction_mask_0407") ;
        }
        size = 16;
        default_action = NoAction_96();
    }
    @name(".int_inst_0811") table _int_inst_5 {
        actions = {
            _nop_7();
            @defaultonly NoAction_97();
        }
        key = {
            hdr.int_header.instruction_mask_0811: exact @name("int_header.instruction_mask_0811") ;
        }
        size = 16;
        default_action = NoAction_97();
    }
    @name(".int_inst_1215") table _int_inst_6 {
        actions = {
            _nop_8();
            @defaultonly NoAction_98();
        }
        key = {
            hdr.int_header.instruction_mask_1215: exact @name("int_header.instruction_mask_1215") ;
        }
        size = 16;
        default_action = NoAction_98();
    }
    @name(".int_meta_header_update") table _int_meta_header_update_0 {
        actions = {
            _int_set_e_bit();
            _int_update_total_hop_cnt();
            @defaultonly NoAction_99();
        }
        key = {
            meta.int_metadata.insert_cnt: ternary @name("int_metadata.insert_cnt") ;
        }
        size = 1;
        default_action = NoAction_99();
    }
    @name(".nop") action _nop_9() {
    }
    @name(".rewrite_ipv4_unicast_mac") action _rewrite_ipv4_unicast_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".rewrite_ipv4_multicast_mac") action _rewrite_ipv4_multicast_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr[47:23] = 25w0x0;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".rewrite_ipv6_unicast_mac") action _rewrite_ipv6_unicast_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit + 8w255;
    }
    @name(".rewrite_ipv6_multicast_mac") action _rewrite_ipv6_multicast_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr[47:32] = 16w0x0;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit + 8w255;
    }
    @name(".rewrite_mpls_mac") action _rewrite_mpls_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.mpls[0].ttl = hdr.mpls[0].ttl + 8w255;
    }
    @name(".mac_rewrite") table _mac_rewrite_0 {
        actions = {
            _nop_9();
            _rewrite_ipv4_unicast_mac();
            _rewrite_ipv4_multicast_mac();
            _rewrite_ipv6_unicast_mac();
            _rewrite_ipv6_multicast_mac();
            _rewrite_mpls_mac();
            @defaultonly NoAction_100();
        }
        key = {
            meta.egress_metadata.smac_idx: exact @name("egress_metadata.smac_idx") ;
            hdr.ipv4.isValid()           : exact @name("ipv4.$valid$") ;
            hdr.ipv6.isValid()           : exact @name("ipv6.$valid$") ;
            hdr.mpls[0].isValid()        : exact @name("mpls[0].$valid$") ;
        }
        size = 512;
        default_action = NoAction_100();
    }
    @name(".nop") action _nop_10() {
    }
    @name(".nop") action _nop_11() {
    }
    @name(".nop") action _nop_12() {
    }
    @name(".nop") action _nop_13() {
    }
    @name(".nop") action _nop_14() {
    }
    @name(".nop") action _nop_15() {
    }
    @name(".nop") action _nop_16() {
    }
    @name(".set_egress_tunnel_vni") action _set_egress_tunnel_vni(bit<24> vnid) {
        meta.tunnel_metadata.vnid = vnid;
    }
    @name(".rewrite_tunnel_dmac") action _rewrite_tunnel_dmac(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name(".rewrite_tunnel_ipv4_dst") action _rewrite_tunnel_ipv4_dst(bit<32> ip) {
        hdr.ipv4.dstAddr = ip;
    }
    @name(".rewrite_tunnel_ipv6_dst") action _rewrite_tunnel_ipv6_dst(bit<128> ip) {
        hdr.ipv6.dstAddr = ip;
    }
    @name(".inner_ipv4_udp_rewrite") action _inner_ipv4_udp_rewrite() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_udp = hdr.udp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.udp.setInvalid();
        hdr.ipv4.setInvalid();
    }
    @name(".inner_ipv4_tcp_rewrite") action _inner_ipv4_tcp_rewrite() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_tcp = hdr.tcp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.tcp.setInvalid();
        hdr.ipv4.setInvalid();
    }
    @name(".inner_ipv4_icmp_rewrite") action _inner_ipv4_icmp_rewrite() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_icmp = hdr.icmp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.icmp.setInvalid();
        hdr.ipv4.setInvalid();
    }
    @name(".inner_ipv4_unknown_rewrite") action _inner_ipv4_unknown_rewrite() {
        hdr.inner_ipv4 = hdr.ipv4;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.ipv4.setInvalid();
    }
    @name(".inner_ipv6_udp_rewrite") action _inner_ipv6_udp_rewrite() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_udp = hdr.udp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.ipv6.setInvalid();
    }
    @name(".inner_ipv6_tcp_rewrite") action _inner_ipv6_tcp_rewrite() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_tcp = hdr.tcp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.tcp.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".inner_ipv6_icmp_rewrite") action _inner_ipv6_icmp_rewrite() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_icmp = hdr.icmp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.icmp.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".inner_ipv6_unknown_rewrite") action _inner_ipv6_unknown_rewrite() {
        hdr.inner_ipv6 = hdr.ipv6;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.ipv6.setInvalid();
    }
    @name(".inner_non_ip_rewrite") action _inner_non_ip_rewrite() {
        meta.egress_metadata.payload_length = (bit<16>)standard_metadata.packet_length + 16w65522;
    }
    @name(".ipv4_vxlan_rewrite") action _ipv4_vxlan_rewrite() {
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
    @name(".ipv6_vxlan_rewrite") action _ipv6_vxlan_rewrite() {
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
    @name(".ipv4_genv_rewrite") action _ipv4_genv_rewrite() {
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
    @name(".ipv6_genv_rewrite") action _ipv6_genv_rewrite() {
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
    @name(".ipv4_nvgre_rewrite") action _ipv4_nvgre_rewrite() {
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
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = 8w47;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w42;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv6_nvgre_rewrite") action _ipv6_nvgre_rewrite() {
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
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = 8w47;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w22;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv4_gre_rewrite") action _ipv4_gre_rewrite() {
        hdr.gre.setValid();
        hdr.gre.proto = hdr.ethernet.etherType;
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = 8w47;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w38;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv6_gre_rewrite") action _ipv6_gre_rewrite() {
        hdr.gre.setValid();
        hdr.gre.proto = 16w0x800;
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = 8w47;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w18;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv4_ipv4_rewrite") action _ipv4_ipv4_rewrite() {
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = 8w4;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w20;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv4_ipv6_rewrite") action _ipv4_ipv6_rewrite() {
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = 8w41;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w40;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv6_ipv4_rewrite") action _ipv6_ipv4_rewrite() {
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = 8w4;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w20;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_ipv6_rewrite") action _ipv6_ipv6_rewrite() {
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = 8w41;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w40;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv4_erspan_t3_rewrite") action _ipv4_erspan_t3_rewrite() {
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
    @name(".ipv6_erspan_t3_rewrite") action _ipv6_erspan_t3_rewrite() {
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
    @name(".mpls_ethernet_push1_rewrite") action _mpls_ethernet_push1_rewrite() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.mpls.push_front(1);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push1_rewrite") action _mpls_ip_push1_rewrite() {
        hdr.mpls.push_front(1);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ethernet_push2_rewrite") action _mpls_ethernet_push2_rewrite() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.mpls.push_front(2);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push2_rewrite") action _mpls_ip_push2_rewrite() {
        hdr.mpls.push_front(2);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ethernet_push3_rewrite") action _mpls_ethernet_push3_rewrite() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.mpls.push_front(3);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push3_rewrite") action _mpls_ip_push3_rewrite() {
        hdr.mpls.push_front(3);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".fabric_rewrite") action _fabric_rewrite(bit<14> tunnel_index) {
        meta.tunnel_metadata.tunnel_index = tunnel_index;
    }
    @name(".set_tunnel_rewrite_details") action _set_tunnel_rewrite_details(bit<16> outer_bd, bit<8> mtu_index, bit<9> smac_idx, bit<14> dmac_idx, bit<9> sip_index, bit<14> dip_index) {
        meta.egress_metadata.outer_bd = outer_bd;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx;
        meta.tunnel_metadata.tunnel_src_index = sip_index;
        meta.tunnel_metadata.tunnel_dst_index = dip_index;
        meta.l3_metadata.mtu_index = mtu_index;
    }
    @name(".set_mpls_rewrite_push1") action _set_mpls_rewrite_push1(bit<20> label1, bit<3> exp1, bit<8> ttl1, bit<9> smac_idx, bit<14> dmac_idx) {
        hdr.mpls[0].label = label1;
        hdr.mpls[0].exp = exp1;
        hdr.mpls[0].bos = 1w0x1;
        hdr.mpls[0].ttl = ttl1;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx;
    }
    @name(".set_mpls_rewrite_push2") action _set_mpls_rewrite_push2(bit<20> label1, bit<3> exp1, bit<8> ttl1, bit<20> label2, bit<3> exp2, bit<8> ttl2, bit<9> smac_idx, bit<14> dmac_idx) {
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
    @name(".set_mpls_rewrite_push3") action _set_mpls_rewrite_push3(bit<20> label1, bit<3> exp1, bit<8> ttl1, bit<20> label2, bit<3> exp2, bit<8> ttl2, bit<20> label3, bit<3> exp3, bit<8> ttl3, bit<9> smac_idx, bit<14> dmac_idx) {
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
    @name(".cpu_rx_rewrite") action _cpu_rx_rewrite() {
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
    @name(".fabric_unicast_rewrite") action _fabric_unicast_rewrite() {
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
    @name(".fabric_multicast_rewrite") action _fabric_multicast_rewrite(bit<16> fabric_mgid) {
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
    @name(".rewrite_tunnel_smac") action _rewrite_tunnel_smac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name(".rewrite_tunnel_ipv4_src") action _rewrite_tunnel_ipv4_src(bit<32> ip) {
        hdr.ipv4.srcAddr = ip;
    }
    @name(".rewrite_tunnel_ipv6_src") action _rewrite_tunnel_ipv6_src(bit<128> ip) {
        hdr.ipv6.srcAddr = ip;
    }
    @name(".egress_vni") table _egress_vni_0 {
        actions = {
            _nop_10();
            _set_egress_tunnel_vni();
            @defaultonly NoAction_101();
        }
        key = {
            meta.egress_metadata.bd                : exact @name("egress_metadata.bd") ;
            meta.tunnel_metadata.egress_tunnel_type: exact @name("tunnel_metadata.egress_tunnel_type") ;
        }
        size = 1024;
        default_action = NoAction_101();
    }
    @name(".tunnel_dmac_rewrite") table _tunnel_dmac_rewrite_0 {
        actions = {
            _nop_11();
            _rewrite_tunnel_dmac();
            @defaultonly NoAction_102();
        }
        key = {
            meta.tunnel_metadata.tunnel_dmac_index: exact @name("tunnel_metadata.tunnel_dmac_index") ;
        }
        size = 1024;
        default_action = NoAction_102();
    }
    @name(".tunnel_dst_rewrite") table _tunnel_dst_rewrite_0 {
        actions = {
            _nop_12();
            _rewrite_tunnel_ipv4_dst();
            _rewrite_tunnel_ipv6_dst();
            @defaultonly NoAction_103();
        }
        key = {
            meta.tunnel_metadata.tunnel_dst_index: exact @name("tunnel_metadata.tunnel_dst_index") ;
        }
        size = 1024;
        default_action = NoAction_103();
    }
    @name(".tunnel_encap_process_inner") table _tunnel_encap_process_inner_0 {
        actions = {
            _inner_ipv4_udp_rewrite();
            _inner_ipv4_tcp_rewrite();
            _inner_ipv4_icmp_rewrite();
            _inner_ipv4_unknown_rewrite();
            _inner_ipv6_udp_rewrite();
            _inner_ipv6_tcp_rewrite();
            _inner_ipv6_icmp_rewrite();
            _inner_ipv6_unknown_rewrite();
            _inner_non_ip_rewrite();
            @defaultonly NoAction_104();
        }
        key = {
            hdr.ipv4.isValid(): exact @name("ipv4.$valid$") ;
            hdr.ipv6.isValid(): exact @name("ipv6.$valid$") ;
            hdr.tcp.isValid() : exact @name("tcp.$valid$") ;
            hdr.udp.isValid() : exact @name("udp.$valid$") ;
            hdr.icmp.isValid(): exact @name("icmp.$valid$") ;
        }
        size = 1024;
        default_action = NoAction_104();
    }
    @name(".tunnel_encap_process_outer") table _tunnel_encap_process_outer_0 {
        actions = {
            _nop_13();
            _ipv4_vxlan_rewrite();
            _ipv6_vxlan_rewrite();
            _ipv4_genv_rewrite();
            _ipv6_genv_rewrite();
            _ipv4_nvgre_rewrite();
            _ipv6_nvgre_rewrite();
            _ipv4_gre_rewrite();
            _ipv6_gre_rewrite();
            _ipv4_ipv4_rewrite();
            _ipv4_ipv6_rewrite();
            _ipv6_ipv4_rewrite();
            _ipv6_ipv6_rewrite();
            _ipv4_erspan_t3_rewrite();
            _ipv6_erspan_t3_rewrite();
            _mpls_ethernet_push1_rewrite();
            _mpls_ip_push1_rewrite();
            _mpls_ethernet_push2_rewrite();
            _mpls_ip_push2_rewrite();
            _mpls_ethernet_push3_rewrite();
            _mpls_ip_push3_rewrite();
            _fabric_rewrite();
            @defaultonly NoAction_105();
        }
        key = {
            meta.tunnel_metadata.egress_tunnel_type : exact @name("tunnel_metadata.egress_tunnel_type") ;
            meta.tunnel_metadata.egress_header_count: exact @name("tunnel_metadata.egress_header_count") ;
            meta.multicast_metadata.replica         : exact @name("multicast_metadata.replica") ;
        }
        size = 1024;
        default_action = NoAction_105();
    }
    @name(".tunnel_rewrite") table _tunnel_rewrite_0 {
        actions = {
            _nop_14();
            _set_tunnel_rewrite_details();
            _set_mpls_rewrite_push1();
            _set_mpls_rewrite_push2();
            _set_mpls_rewrite_push3();
            _cpu_rx_rewrite();
            _fabric_unicast_rewrite();
            _fabric_multicast_rewrite();
            @defaultonly NoAction_106();
        }
        key = {
            meta.tunnel_metadata.tunnel_index: exact @name("tunnel_metadata.tunnel_index") ;
        }
        size = 1024;
        default_action = NoAction_106();
    }
    @name(".tunnel_smac_rewrite") table _tunnel_smac_rewrite_0 {
        actions = {
            _nop_15();
            _rewrite_tunnel_smac();
            @defaultonly NoAction_107();
        }
        key = {
            meta.tunnel_metadata.tunnel_smac_index: exact @name("tunnel_metadata.tunnel_smac_index") ;
        }
        size = 1024;
        default_action = NoAction_107();
    }
    @name(".tunnel_src_rewrite") table _tunnel_src_rewrite_0 {
        actions = {
            _nop_16();
            _rewrite_tunnel_ipv4_src();
            _rewrite_tunnel_ipv6_src();
            @defaultonly NoAction_108();
        }
        key = {
            meta.tunnel_metadata.tunnel_src_index: exact @name("tunnel_metadata.tunnel_src_index") ;
        }
        size = 1024;
        default_action = NoAction_108();
    }
    @name(".int_update_vxlan_gpe_ipv4") action _int_update_vxlan_gpe_ipv4() {
        hdr.ipv4.totalLen = hdr.ipv4.totalLen + meta.int_metadata.insert_byte_cnt;
        hdr.udp.length_ = hdr.udp.length_ + meta.int_metadata.insert_byte_cnt;
        hdr.vxlan_gpe_int_header.len = hdr.vxlan_gpe_int_header.len + meta.int_metadata.gpe_int_hdr_len8;
    }
    @name(".nop") action _nop_17() {
    }
    @name(".int_outer_encap") table _int_outer_encap_0 {
        actions = {
            _int_update_vxlan_gpe_ipv4();
            _nop_17();
            @defaultonly NoAction_109();
        }
        key = {
            hdr.ipv4.isValid()                     : exact @name("ipv4.$valid$") ;
            hdr.vxlan_gpe.isValid()                : exact @name("vxlan_gpe.$valid$") ;
            meta.int_metadata_i2e.source           : exact @name("int_metadata_i2e.source") ;
            meta.tunnel_metadata.egress_tunnel_type: ternary @name("tunnel_metadata.egress_tunnel_type") ;
        }
        size = 8;
        default_action = NoAction_109();
    }
    @name(".set_egress_packet_vlan_untagged") action _set_egress_packet_vlan_untagged() {
    }
    @name(".set_egress_packet_vlan_tagged") action _set_egress_packet_vlan_tagged(bit<12> vlan_id) {
        hdr.vlan_tag_[0].setValid();
        hdr.vlan_tag_[0].etherType = hdr.ethernet.etherType;
        hdr.vlan_tag_[0].vid = vlan_id;
        hdr.ethernet.etherType = 16w0x8100;
    }
    @name(".set_egress_packet_vlan_double_tagged") action _set_egress_packet_vlan_double_tagged(bit<12> s_tag, bit<12> c_tag) {
        hdr.vlan_tag_[1].setValid();
        hdr.vlan_tag_[0].setValid();
        hdr.vlan_tag_[1].etherType = hdr.ethernet.etherType;
        hdr.vlan_tag_[1].vid = c_tag;
        hdr.vlan_tag_[0].etherType = 16w0x8100;
        hdr.vlan_tag_[0].vid = s_tag;
        hdr.ethernet.etherType = 16w0x9100;
    }
    @name(".egress_vlan_xlate") table _egress_vlan_xlate_0 {
        actions = {
            _set_egress_packet_vlan_untagged();
            _set_egress_packet_vlan_tagged();
            _set_egress_packet_vlan_double_tagged();
            @defaultonly NoAction_110();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
            meta.egress_metadata.bd      : exact @name("egress_metadata.bd") ;
        }
        size = 1024;
        default_action = NoAction_110();
    }
    @name(".set_egress_filter_drop") action _set_egress_filter_drop() {
        mark_to_drop();
    }
    @name(".set_egress_ifindex") action _set_egress_ifindex(bit<16> egress_ifindex) {
        meta.egress_filter_metadata.ifindex = meta.ingress_metadata.ifindex ^ egress_ifindex;
        meta.egress_filter_metadata.bd = meta.ingress_metadata.outer_bd ^ meta.egress_metadata.outer_bd;
        meta.egress_filter_metadata.inner_bd = meta.ingress_metadata.bd ^ meta.egress_metadata.bd;
    }
    @name(".egress_filter") table _egress_filter_0 {
        actions = {
            _set_egress_filter_drop();
            @defaultonly NoAction_111();
        }
        default_action = NoAction_111();
    }
    @name(".egress_lag") table _egress_lag_0 {
        actions = {
            _set_egress_ifindex();
            @defaultonly NoAction_112();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
        }
        default_action = NoAction_112();
    }
    @name(".nop") action _nop_18() {
    }
    @name(".egress_mirror") action _egress_mirror(bit<32> session_id) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        clone3<tuple_0>(CloneType.E2E, session_id, { meta.i2e_metadata.ingress_tstamp, (bit<16>)session_id });
    }
    @name(".egress_mirror_drop") action _egress_mirror_drop(bit<32> session_id) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        clone3<tuple_0>(CloneType.E2E, session_id, { meta.i2e_metadata.ingress_tstamp, (bit<16>)session_id });
        mark_to_drop();
    }
    @name(".egress_redirect_to_cpu") action _egress_redirect_to_cpu(bit<16> reason_code) {
        meta.fabric_metadata.reason_code = reason_code;
        clone3<tuple_1>(CloneType.E2E, 32w250, { meta.ingress_metadata.bd, meta.ingress_metadata.ifindex, reason_code, meta.ingress_metadata.ingress_port });
        mark_to_drop();
    }
    @name(".egress_acl") table _egress_acl_0 {
        actions = {
            _nop_18();
            _egress_mirror();
            _egress_mirror_drop();
            _egress_redirect_to_cpu();
            @defaultonly NoAction_113();
        }
        key = {
            standard_metadata.egress_port          : ternary @name("standard_metadata.egress_port") ;
            meta.intrinsic_metadata.deflection_flag: ternary @name("intrinsic_metadata.deflection_flag") ;
            meta.l3_metadata.l3_mtu_check          : ternary @name("l3_metadata.l3_mtu_check") ;
        }
        size = 512;
        default_action = NoAction_113();
    }
    apply {
        if (meta.intrinsic_metadata.deflection_flag == 1w0 && meta.egress_metadata.bypass == 1w0) {
            if (standard_metadata.instance_type != 32w0 && standard_metadata.instance_type != 32w5) 
                mirror.apply();
            else 
                if (meta.intrinsic_metadata.egress_rid != 16w0) {
                    _rid_0.apply();
                    _replica_type_0.apply();
                }
            switch (egress_port_mapping.apply().action_run) {
                egress_port_type_normal_0: {
                    if (standard_metadata.instance_type == 32w0 || standard_metadata.instance_type == 32w5) 
                        _vlan_decap_0.apply();
                    if (meta.tunnel_metadata.tunnel_terminate == 1w1) 
                        if (meta.multicast_metadata.inner_replica == 1w1 || meta.multicast_metadata.replica == 1w0) {
                            _tunnel_decap_process_outer_0.apply();
                            _tunnel_decap_process_inner_0.apply();
                        }
                    _egress_bd_map_0.apply();
                    _rewrite_0.apply();
                    switch (_int_insert_0.apply().action_run) {
                        _int_transit: {
                            if (meta.int_metadata.insert_cnt != 8w0) {
                                _int_inst_3.apply();
                                _int_inst_4.apply();
                                _int_inst_5.apply();
                                _int_inst_6.apply();
                                _int_bos_0.apply();
                            }
                            _int_meta_header_update_0.apply();
                        }
                    }

                    if (meta.egress_metadata.routed == 1w1) 
                        _mac_rewrite_0.apply();
                }
            }

            if (meta.fabric_metadata.fabric_header_present == 1w0 && meta.tunnel_metadata.egress_tunnel_type != 5w0) {
                _egress_vni_0.apply();
                if (meta.tunnel_metadata.egress_tunnel_type != 5w15 && meta.tunnel_metadata.egress_tunnel_type != 5w16) 
                    _tunnel_encap_process_inner_0.apply();
                _tunnel_encap_process_outer_0.apply();
                _tunnel_rewrite_0.apply();
                _tunnel_src_rewrite_0.apply();
                _tunnel_dst_rewrite_0.apply();
                _tunnel_smac_rewrite_0.apply();
                _tunnel_dmac_rewrite_0.apply();
            }
            if (meta.int_metadata.insert_cnt != 8w0) 
                _int_outer_encap_0.apply();
            if (meta.egress_metadata.port_type == 2w0) 
                _egress_vlan_xlate_0.apply();
            _egress_lag_0.apply();
            if (meta.multicast_metadata.inner_replica == 1w1) 
                if (meta.tunnel_metadata.egress_tunnel_type != 5w0 && meta.egress_filter_metadata.inner_bd == 16w0 || meta.egress_filter_metadata.ifindex == 16w0 && meta.egress_filter_metadata.bd == 16w0) 
                    _egress_filter_0.apply();
        }
        _egress_acl_0.apply();
    }
}

@name("mac_learn_digest") struct mac_learn_digest {
    bit<16> bd;
    bit<48> lkp_mac_sa;
    bit<16> ifindex;
}

struct tuple_2 {
    bit<32> field_5;
    bit<32> field_6;
    bit<8>  field_7;
    bit<16> field_8;
    bit<16> field_9;
}

struct tuple_3 {
    bit<48> field_10;
    bit<48> field_11;
    bit<32> field_12;
    bit<32> field_13;
    bit<8>  field_14;
    bit<16> field_15;
    bit<16> field_16;
}

struct tuple_4 {
    bit<128> field_17;
    bit<128> field_18;
    bit<8>   field_19;
    bit<16>  field_20;
    bit<16>  field_21;
}

struct tuple_5 {
    bit<48>  field_22;
    bit<48>  field_23;
    bit<128> field_24;
    bit<128> field_25;
    bit<8>   field_26;
    bit<16>  field_27;
    bit<16>  field_28;
}

struct tuple_6 {
    bit<16> field_29;
    bit<48> field_30;
    bit<48> field_31;
    bit<16> field_32;
}

struct tuple_7 {
    bit<16> field_33;
    bit<8>  field_34;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("NoAction") action NoAction_114() {
    }
    @name("NoAction") action NoAction_115() {
    }
    @name("NoAction") action NoAction_116() {
    }
    @name("NoAction") action NoAction_117() {
    }
    @name("NoAction") action NoAction_118() {
    }
    @name("NoAction") action NoAction_119() {
    }
    @name("NoAction") action NoAction_120() {
    }
    @name("NoAction") action NoAction_121() {
    }
    @name("NoAction") action NoAction_122() {
    }
    @name("NoAction") action NoAction_123() {
    }
    @name("NoAction") action NoAction_124() {
    }
    @name("NoAction") action NoAction_125() {
    }
    @name("NoAction") action NoAction_126() {
    }
    @name("NoAction") action NoAction_127() {
    }
    @name("NoAction") action NoAction_128() {
    }
    @name("NoAction") action NoAction_129() {
    }
    @name("NoAction") action NoAction_130() {
    }
    @name("NoAction") action NoAction_131() {
    }
    @name("NoAction") action NoAction_132() {
    }
    @name("NoAction") action NoAction_133() {
    }
    @name("NoAction") action NoAction_134() {
    }
    @name("NoAction") action NoAction_135() {
    }
    @name("NoAction") action NoAction_136() {
    }
    @name("NoAction") action NoAction_137() {
    }
    @name("NoAction") action NoAction_138() {
    }
    @name("NoAction") action NoAction_139() {
    }
    @name("NoAction") action NoAction_140() {
    }
    @name("NoAction") action NoAction_141() {
    }
    @name("NoAction") action NoAction_142() {
    }
    @name("NoAction") action NoAction_143() {
    }
    @name("NoAction") action NoAction_144() {
    }
    @name("NoAction") action NoAction_145() {
    }
    @name("NoAction") action NoAction_146() {
    }
    @name("NoAction") action NoAction_147() {
    }
    @name("NoAction") action NoAction_148() {
    }
    @name("NoAction") action NoAction_149() {
    }
    @name("NoAction") action NoAction_150() {
    }
    @name("NoAction") action NoAction_151() {
    }
    @name("NoAction") action NoAction_152() {
    }
    @name("NoAction") action NoAction_153() {
    }
    @name("NoAction") action NoAction_154() {
    }
    @name("NoAction") action NoAction_155() {
    }
    @name("NoAction") action NoAction_156() {
    }
    @name("NoAction") action NoAction_157() {
    }
    @name("NoAction") action NoAction_158() {
    }
    @name("NoAction") action NoAction_159() {
    }
    @name("NoAction") action NoAction_160() {
    }
    @name("NoAction") action NoAction_161() {
    }
    @name("NoAction") action NoAction_162() {
    }
    @name("NoAction") action NoAction_163() {
    }
    @name("NoAction") action NoAction_164() {
    }
    @name("NoAction") action NoAction_165() {
    }
    @name("NoAction") action NoAction_166() {
    }
    @name("NoAction") action NoAction_167() {
    }
    @name(".rmac_hit") action rmac_hit_0() {
        meta.l3_metadata.rmac_hit = 1w1;
    }
    @name(".rmac_miss") action rmac_miss_0() {
        meta.l3_metadata.rmac_hit = 1w0;
    }
    @name(".rmac") table rmac {
        actions = {
            rmac_hit_0();
            rmac_miss_0();
            @defaultonly NoAction_114();
        }
        key = {
            meta.l3_metadata.rmac_group: exact @name("l3_metadata.rmac_group") ;
            meta.l2_metadata.lkp_mac_da: exact @name("l2_metadata.lkp_mac_da") ;
        }
        size = 1024;
        default_action = NoAction_114();
    }
    @name(".set_ifindex") action _set_ifindex(bit<16> ifindex, bit<15> if_label, bit<2> port_type) {
        meta.ingress_metadata.ifindex = ifindex;
        meta.acl_metadata.if_label = if_label;
        meta.ingress_metadata.port_type = port_type;
    }
    @name(".ingress_port_mapping") table _ingress_port_mapping_0 {
        actions = {
            _set_ifindex();
            @defaultonly NoAction_115();
        }
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port") ;
        }
        size = 288;
        default_action = NoAction_115();
    }
    @name(".malformed_outer_ethernet_packet") action _malformed_outer_ethernet_packet(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_untagged") action _set_valid_outer_unicast_packet_untagged() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_single_tagged") action _set_valid_outer_unicast_packet_single_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_double_tagged") action _set_valid_outer_unicast_packet_double_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_qinq_tagged") action _set_valid_outer_unicast_packet_qinq_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_untagged") action _set_valid_outer_multicast_packet_untagged() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_single_tagged") action _set_valid_outer_multicast_packet_single_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_double_tagged") action _set_valid_outer_multicast_packet_double_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_qinq_tagged") action _set_valid_outer_multicast_packet_qinq_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_untagged") action _set_valid_outer_broadcast_packet_untagged() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_single_tagged") action _set_valid_outer_broadcast_packet_single_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_double_tagged") action _set_valid_outer_broadcast_packet_double_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_qinq_tagged") action _set_valid_outer_broadcast_packet_qinq_tagged() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".validate_outer_ethernet") table _validate_outer_ethernet_0 {
        actions = {
            _malformed_outer_ethernet_packet();
            _set_valid_outer_unicast_packet_untagged();
            _set_valid_outer_unicast_packet_single_tagged();
            _set_valid_outer_unicast_packet_double_tagged();
            _set_valid_outer_unicast_packet_qinq_tagged();
            _set_valid_outer_multicast_packet_untagged();
            _set_valid_outer_multicast_packet_single_tagged();
            _set_valid_outer_multicast_packet_double_tagged();
            _set_valid_outer_multicast_packet_qinq_tagged();
            _set_valid_outer_broadcast_packet_untagged();
            _set_valid_outer_broadcast_packet_single_tagged();
            _set_valid_outer_broadcast_packet_double_tagged();
            _set_valid_outer_broadcast_packet_qinq_tagged();
            @defaultonly NoAction_116();
        }
        key = {
            meta.l2_metadata.lkp_mac_sa: ternary @name("l2_metadata.lkp_mac_sa") ;
            meta.l2_metadata.lkp_mac_da: ternary @name("l2_metadata.lkp_mac_da") ;
            hdr.vlan_tag_[0].isValid() : exact @name("vlan_tag_[0].$valid$") ;
            hdr.vlan_tag_[1].isValid() : exact @name("vlan_tag_[1].$valid$") ;
        }
        size = 512;
        default_action = NoAction_116();
    }
    @name(".set_valid_outer_ipv4_packet") action _set_valid_outer_ipv4_packet_0() {
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.l3_metadata.lkp_ip_tc = hdr.ipv4.diffserv;
        meta.l3_metadata.lkp_ip_version = 4w4;
    }
    @name(".set_malformed_outer_ipv4_packet") action _set_malformed_outer_ipv4_packet_0(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
    }
    @name(".validate_outer_ipv4_packet") table _validate_outer_ipv4_packet {
        actions = {
            _set_valid_outer_ipv4_packet_0();
            _set_malformed_outer_ipv4_packet_0();
            @defaultonly NoAction_117();
        }
        key = {
            hdr.ipv4.version                     : ternary @name("ipv4.version") ;
            meta.l3_metadata.lkp_ip_ttl          : ternary @name("l3_metadata.lkp_ip_ttl") ;
            meta.ipv4_metadata.lkp_ipv4_sa[31:24]: ternary @name("ipv4_metadata.lkp_ipv4_sa[31:24]") ;
        }
        size = 512;
        default_action = NoAction_117();
    }
    @name(".set_valid_outer_ipv6_packet") action _set_valid_outer_ipv6_packet_0() {
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.l3_metadata.lkp_ip_tc = hdr.ipv6.trafficClass;
        meta.l3_metadata.lkp_ip_version = 4w6;
    }
    @name(".set_malformed_outer_ipv6_packet") action _set_malformed_outer_ipv6_packet_0(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
    }
    @name(".validate_outer_ipv6_packet") table _validate_outer_ipv6_packet {
        actions = {
            _set_valid_outer_ipv6_packet_0();
            _set_malformed_outer_ipv6_packet_0();
            @defaultonly NoAction_118();
        }
        key = {
            hdr.ipv6.version                       : ternary @name("ipv6.version") ;
            meta.l3_metadata.lkp_ip_ttl            : ternary @name("l3_metadata.lkp_ip_ttl") ;
            meta.ipv6_metadata.lkp_ipv6_sa[127:112]: ternary @name("ipv6_metadata.lkp_ipv6_sa[127:112]") ;
        }
        size = 512;
        default_action = NoAction_118();
    }
    @name(".set_valid_mpls_label1") action _set_valid_mpls_label1_0() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[0].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[0].exp;
    }
    @name(".set_valid_mpls_label2") action _set_valid_mpls_label2_0() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[1].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[1].exp;
    }
    @name(".set_valid_mpls_label3") action _set_valid_mpls_label3_0() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[2].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[2].exp;
    }
    @name(".validate_mpls_packet") table _validate_mpls_packet {
        actions = {
            _set_valid_mpls_label1_0();
            _set_valid_mpls_label2_0();
            _set_valid_mpls_label3_0();
            @defaultonly NoAction_119();
        }
        key = {
            hdr.mpls[0].label    : ternary @name("mpls[0].label") ;
            hdr.mpls[0].bos      : ternary @name("mpls[0].bos") ;
            hdr.mpls[0].isValid(): exact @name("mpls[0].$valid$") ;
            hdr.mpls[1].label    : ternary @name("mpls[1].label") ;
            hdr.mpls[1].bos      : ternary @name("mpls[1].bos") ;
            hdr.mpls[1].isValid(): exact @name("mpls[1].$valid$") ;
            hdr.mpls[2].label    : ternary @name("mpls[2].label") ;
            hdr.mpls[2].bos      : ternary @name("mpls[2].bos") ;
            hdr.mpls[2].isValid(): exact @name("mpls[2].$valid$") ;
        }
        size = 512;
        default_action = NoAction_119();
    }
    @name(".storm_control_meter") meter(32w1024, MeterType.bytes) _storm_control_meter_0;
    @name(".nop") action _nop_19() {
    }
    @name(".set_storm_control_meter") action _set_storm_control_meter(bit<32> meter_idx) {
        _storm_control_meter_0.execute_meter<bit<1>>(meter_idx, meta.security_metadata.storm_control_color);
    }
    @name(".storm_control") table _storm_control_0 {
        actions = {
            _nop_19();
            _set_storm_control_meter();
            @defaultonly NoAction_120();
        }
        key = {
            meta.ingress_metadata.ifindex: exact @name("ingress_metadata.ifindex") ;
            meta.l2_metadata.lkp_pkt_type: ternary @name("l2_metadata.lkp_pkt_type") ;
        }
        size = 512;
        default_action = NoAction_120();
    }
    @name(".set_bd") action _set_bd(bit<16> bd, bit<12> vrf, bit<10> rmac_group, bit<1> ipv4_unicast_enabled, bit<1> ipv6_unicast_enabled, bit<2> ipv4_urpf_mode, bit<2> ipv6_urpf_mode, bit<1> igmp_snooping_enabled, bit<1> mld_snooping_enabled, bit<16> bd_label, bit<10> stp_group, bit<16> stats_idx, bit<1> learning_enabled) {
        meta.l3_metadata.vrf = vrf;
        meta.ipv4_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled;
        meta.ipv6_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled;
        meta.ipv4_metadata.ipv4_urpf_mode = ipv4_urpf_mode;
        meta.ipv6_metadata.ipv6_urpf_mode = ipv6_urpf_mode;
        meta.l3_metadata.rmac_group = rmac_group;
        meta.acl_metadata.bd_label = bd_label;
        meta.ingress_metadata.bd = bd;
        meta.ingress_metadata.outer_bd = bd;
        meta.l2_metadata.stp_group = stp_group;
        meta.l2_metadata.bd_stats_idx = stats_idx;
        meta.l2_metadata.learning_enabled = learning_enabled;
        meta.multicast_metadata.igmp_snooping_enabled = igmp_snooping_enabled;
        meta.multicast_metadata.mld_snooping_enabled = mld_snooping_enabled;
    }
    @name(".port_vlan_mapping_miss") action _port_vlan_mapping_miss() {
        meta.l2_metadata.port_vlan_mapping_miss = 1w1;
    }
    @name(".port_vlan_mapping") table _port_vlan_mapping_0 {
        actions = {
            _set_bd();
            _port_vlan_mapping_miss();
            @defaultonly NoAction_121();
        }
        key = {
            meta.ingress_metadata.ifindex: exact @name("ingress_metadata.ifindex") ;
            hdr.vlan_tag_[0].isValid()   : exact @name("vlan_tag_[0].$valid$") ;
            hdr.vlan_tag_[0].vid         : exact @name("vlan_tag_[0].vid") ;
            hdr.vlan_tag_[1].isValid()   : exact @name("vlan_tag_[1].$valid$") ;
            hdr.vlan_tag_[1].vid         : exact @name("vlan_tag_[1].vid") ;
        }
        size = 4096;
        implementation = bd_action_profile;
        default_action = NoAction_121();
    }
    @name(".set_stp_state") action _set_stp_state(bit<3> stp_state) {
        meta.l2_metadata.stp_state = stp_state;
    }
    @name(".spanning_tree") table _spanning_tree_0 {
        actions = {
            _set_stp_state();
            @defaultonly NoAction_122();
        }
        key = {
            meta.ingress_metadata.ifindex: exact @name("ingress_metadata.ifindex") ;
            meta.l2_metadata.stp_group   : exact @name("l2_metadata.stp_group") ;
        }
        size = 1024;
        default_action = NoAction_122();
    }
    @name(".on_miss") action _on_miss_1() {
    }
    @name(".ipsg_miss") action _ipsg_miss() {
        meta.security_metadata.ipsg_check_fail = 1w1;
    }
    @name(".ipsg") table _ipsg_0 {
        actions = {
            _on_miss_1();
            @defaultonly NoAction_123();
        }
        key = {
            meta.ingress_metadata.ifindex : exact @name("ingress_metadata.ifindex") ;
            meta.ingress_metadata.bd      : exact @name("ingress_metadata.bd") ;
            meta.l2_metadata.lkp_mac_sa   : exact @name("l2_metadata.lkp_mac_sa") ;
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("ipv4_metadata.lkp_ipv4_sa") ;
        }
        size = 1024;
        default_action = NoAction_123();
    }
    @name(".ipsg_permit_special") table _ipsg_permit_special_0 {
        actions = {
            _ipsg_miss();
            @defaultonly NoAction_124();
        }
        key = {
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport") ;
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da") ;
        }
        size = 512;
        default_action = NoAction_124();
    }
    @name(".on_miss") action _on_miss_2() {
    }
    @name(".outer_rmac_hit") action _outer_rmac_hit() {
        meta.l3_metadata.rmac_hit = 1w1;
    }
    @name(".nop") action _nop_20() {
    }
    @name(".terminate_tunnel_inner_non_ip") action _terminate_tunnel_inner_non_ip(bit<16> bd, bit<16> bd_label, bit<16> stats_idx) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.ingress_metadata.bd = bd;
        meta.l3_metadata.lkp_ip_type = 2w0;
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.acl_metadata.bd_label = bd_label;
        meta.l2_metadata.bd_stats_idx = stats_idx;
    }
    @name(".terminate_tunnel_inner_ethernet_ipv4") action _terminate_tunnel_inner_ethernet_ipv4(bit<16> bd, bit<12> vrf, bit<10> rmac_group, bit<16> bd_label, bit<1> ipv4_unicast_enabled, bit<2> ipv4_urpf_mode, bit<1> igmp_snooping_enabled, bit<16> stats_idx) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.ingress_metadata.bd = bd;
        meta.l3_metadata.vrf = vrf;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.inner_ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.inner_ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv4.protocol;
        meta.l3_metadata.lkp_ip_ttl = hdr.inner_ipv4.ttl;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
        meta.ipv4_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled;
        meta.ipv4_metadata.ipv4_urpf_mode = ipv4_urpf_mode;
        meta.l3_metadata.rmac_group = rmac_group;
        meta.acl_metadata.bd_label = bd_label;
        meta.l2_metadata.bd_stats_idx = stats_idx;
        meta.multicast_metadata.igmp_snooping_enabled = igmp_snooping_enabled;
    }
    @name(".terminate_tunnel_inner_ipv4") action _terminate_tunnel_inner_ipv4(bit<12> vrf, bit<10> rmac_group, bit<2> ipv4_urpf_mode, bit<1> ipv4_unicast_enabled) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.l3_metadata.vrf = vrf;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.inner_ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.inner_ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv4.protocol;
        meta.l3_metadata.lkp_ip_ttl = hdr.inner_ipv4.ttl;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
        meta.ipv4_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled;
        meta.ipv4_metadata.ipv4_urpf_mode = ipv4_urpf_mode;
        meta.l3_metadata.rmac_group = rmac_group;
    }
    @name(".terminate_tunnel_inner_ethernet_ipv6") action _terminate_tunnel_inner_ethernet_ipv6(bit<16> bd, bit<12> vrf, bit<10> rmac_group, bit<16> bd_label, bit<1> ipv6_unicast_enabled, bit<2> ipv6_urpf_mode, bit<1> mld_snooping_enabled, bit<16> stats_idx) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.ingress_metadata.bd = bd;
        meta.l3_metadata.vrf = vrf;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv6.version;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv6.nextHdr;
        meta.l3_metadata.lkp_ip_ttl = hdr.inner_ipv6.hopLimit;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv6.trafficClass;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
        meta.ipv6_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled;
        meta.ipv6_metadata.ipv6_urpf_mode = ipv6_urpf_mode;
        meta.l3_metadata.rmac_group = rmac_group;
        meta.acl_metadata.bd_label = bd_label;
        meta.l2_metadata.bd_stats_idx = stats_idx;
        meta.multicast_metadata.mld_snooping_enabled = mld_snooping_enabled;
    }
    @name(".terminate_tunnel_inner_ipv6") action _terminate_tunnel_inner_ipv6(bit<12> vrf, bit<10> rmac_group, bit<1> ipv6_unicast_enabled, bit<2> ipv6_urpf_mode) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.l3_metadata.vrf = vrf;
        meta.qos_metadata.outer_dscp = meta.l3_metadata.lkp_ip_tc;
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv6.version;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv6.nextHdr;
        meta.l3_metadata.lkp_ip_ttl = hdr.inner_ipv6.hopLimit;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv6.trafficClass;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
        meta.ipv6_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled;
        meta.ipv6_metadata.ipv6_urpf_mode = ipv6_urpf_mode;
        meta.l3_metadata.rmac_group = rmac_group;
    }
    @name(".outer_rmac") table _outer_rmac_0 {
        actions = {
            _on_miss_2();
            _outer_rmac_hit();
            @defaultonly NoAction_125();
        }
        key = {
            meta.l3_metadata.rmac_group: exact @name("l3_metadata.rmac_group") ;
            meta.l2_metadata.lkp_mac_da: exact @name("l2_metadata.lkp_mac_da") ;
        }
        size = 1024;
        default_action = NoAction_125();
    }
    @name(".tunnel") table _tunnel_0 {
        actions = {
            _nop_20();
            _terminate_tunnel_inner_non_ip();
            _terminate_tunnel_inner_ethernet_ipv4();
            _terminate_tunnel_inner_ipv4();
            _terminate_tunnel_inner_ethernet_ipv6();
            _terminate_tunnel_inner_ipv6();
            @defaultonly NoAction_126();
        }
        key = {
            meta.tunnel_metadata.tunnel_vni         : exact @name("tunnel_metadata.tunnel_vni") ;
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type") ;
            hdr.inner_ipv4.isValid()                : exact @name("inner_ipv4.$valid$") ;
            hdr.inner_ipv6.isValid()                : exact @name("inner_ipv6.$valid$") ;
        }
        size = 1024;
        default_action = NoAction_126();
    }
    @name(".nop") action _nop_21() {
    }
    @name(".set_tunnel_termination_flag") action _set_tunnel_termination_flag_1() {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".on_miss") action _on_miss_3() {
    }
    @name(".src_vtep_hit") action _src_vtep_hit_1(bit<16> ifindex) {
        meta.ingress_metadata.ifindex = ifindex;
    }
    @name(".ipv4_dest_vtep") table _ipv4_dest_vtep {
        actions = {
            _nop_21();
            _set_tunnel_termination_flag_1();
            @defaultonly NoAction_127();
        }
        key = {
            meta.l3_metadata.vrf                    : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_da          : exact @name("ipv4_metadata.lkp_ipv4_da") ;
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type") ;
        }
        size = 1024;
        default_action = NoAction_127();
    }
    @name(".ipv4_src_vtep") table _ipv4_src_vtep {
        actions = {
            _on_miss_3();
            _src_vtep_hit_1();
            @defaultonly NoAction_128();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("ipv4_metadata.lkp_ipv4_sa") ;
        }
        size = 1024;
        default_action = NoAction_128();
    }
    @name(".nop") action _nop_22() {
    }
    @name(".set_tunnel_termination_flag") action _set_tunnel_termination_flag_2() {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".on_miss") action _on_miss_4() {
    }
    @name(".src_vtep_hit") action _src_vtep_hit_2(bit<16> ifindex) {
        meta.ingress_metadata.ifindex = ifindex;
    }
    @name(".ipv6_dest_vtep") table _ipv6_dest_vtep {
        actions = {
            _nop_22();
            _set_tunnel_termination_flag_2();
            @defaultonly NoAction_129();
        }
        key = {
            meta.l3_metadata.vrf                    : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_da          : exact @name("ipv6_metadata.lkp_ipv6_da") ;
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type") ;
        }
        size = 1024;
        default_action = NoAction_129();
    }
    @name(".ipv6_src_vtep") table _ipv6_src_vtep {
        actions = {
            _on_miss_4();
            _src_vtep_hit_2();
            @defaultonly NoAction_130();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_sa: exact @name("ipv6_metadata.lkp_ipv6_sa") ;
        }
        size = 1024;
        default_action = NoAction_130();
    }
    @name(".terminate_eompls") action _terminate_eompls_0(bit<16> bd, bit<5> tunnel_type) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type;
        meta.ingress_metadata.bd = bd;
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_vpls") action _terminate_vpls_0(bit<16> bd, bit<5> tunnel_type) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type;
        meta.ingress_metadata.bd = bd;
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_ipv4_over_mpls") action _terminate_ipv4_over_mpls_0(bit<12> vrf, bit<5> tunnel_type) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type;
        meta.l3_metadata.vrf = vrf;
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.inner_ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.inner_ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv4.protocol;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
        meta.l3_metadata.lkp_ip_ttl = hdr.inner_ipv4.ttl;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
    }
    @name(".terminate_ipv6_over_mpls") action _terminate_ipv6_over_mpls_0(bit<12> vrf, bit<5> tunnel_type) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type;
        meta.l3_metadata.vrf = vrf;
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv6.version;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv6.nextHdr;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv6.trafficClass;
        meta.l3_metadata.lkp_ip_ttl = hdr.inner_ipv6.hopLimit;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
    }
    @name(".terminate_pw") action _terminate_pw_0(bit<16> ifindex) {
        meta.ingress_metadata.egress_ifindex = ifindex;
    }
    @name(".forward_mpls") action _forward_mpls_0(bit<16> nexthop_index) {
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
        meta.l3_metadata.fib_hit = 1w1;
    }
    @name(".mpls") table _mpls {
        actions = {
            _terminate_eompls_0();
            _terminate_vpls_0();
            _terminate_ipv4_over_mpls_0();
            _terminate_ipv6_over_mpls_0();
            _terminate_pw_0();
            _forward_mpls_0();
            @defaultonly NoAction_131();
        }
        key = {
            meta.tunnel_metadata.mpls_label: exact @name("tunnel_metadata.mpls_label") ;
            hdr.inner_ipv4.isValid()       : exact @name("inner_ipv4.$valid$") ;
            hdr.inner_ipv6.isValid()       : exact @name("inner_ipv6.$valid$") ;
        }
        size = 1024;
        default_action = NoAction_131();
    }
    @name(".nop") action _nop_23() {
    }
    @name(".set_unicast") action _set_unicast() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
    }
    @name(".set_unicast_and_ipv6_src_is_link_local") action _set_unicast_and_ipv6_src_is_link_local() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.ipv6_metadata.ipv6_src_is_link_local = 1w1;
    }
    @name(".set_multicast") action _set_multicast() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w1;
    }
    @name(".set_multicast_and_ipv6_src_is_link_local") action _set_multicast_and_ipv6_src_is_link_local() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.ipv6_metadata.ipv6_src_is_link_local = 1w1;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w1;
    }
    @name(".set_broadcast") action _set_broadcast() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w2;
    }
    @name(".set_malformed_packet") action _set_malformed_packet(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
    }
    @name(".validate_packet") table _validate_packet_0 {
        actions = {
            _nop_23();
            _set_unicast();
            _set_unicast_and_ipv6_src_is_link_local();
            _set_multicast();
            _set_multicast_and_ipv6_src_is_link_local();
            _set_broadcast();
            _set_malformed_packet();
            @defaultonly NoAction_132();
        }
        key = {
            meta.l2_metadata.lkp_mac_sa[40:40]     : ternary @name("l2_metadata.lkp_mac_sa[40:40]") ;
            meta.l2_metadata.lkp_mac_da            : ternary @name("l2_metadata.lkp_mac_da") ;
            meta.l3_metadata.lkp_ip_type           : ternary @name("l3_metadata.lkp_ip_type") ;
            meta.l3_metadata.lkp_ip_ttl            : ternary @name("l3_metadata.lkp_ip_ttl") ;
            meta.l3_metadata.lkp_ip_version        : ternary @name("l3_metadata.lkp_ip_version") ;
            meta.ipv4_metadata.lkp_ipv4_sa[31:24]  : ternary @name("ipv4_metadata.lkp_ipv4_sa[31:24]") ;
            meta.ipv6_metadata.lkp_ipv6_sa[127:112]: ternary @name("ipv6_metadata.lkp_ipv6_sa[127:112]") ;
        }
        size = 512;
        default_action = NoAction_132();
    }
    @name(".nop") action _nop_24() {
    }
    @name(".nop") action _nop_25() {
    }
    @name(".dmac_hit") action _dmac_hit(bit<16> ifindex) {
        meta.ingress_metadata.egress_ifindex = ifindex;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex;
    }
    @name(".dmac_multicast_hit") action _dmac_multicast_hit(bit<16> mc_index) {
        meta.intrinsic_metadata.mcast_grp = mc_index;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".dmac_miss") action _dmac_miss() {
        meta.ingress_metadata.egress_ifindex = 16w65535;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".dmac_redirect_nexthop") action _dmac_redirect_nexthop(bit<16> nexthop_index) {
        meta.l2_metadata.l2_redirect = 1w1;
        meta.l2_metadata.l2_nexthop = nexthop_index;
        meta.l2_metadata.l2_nexthop_type = 1w0;
    }
    @name(".dmac_redirect_ecmp") action _dmac_redirect_ecmp(bit<16> ecmp_index) {
        meta.l2_metadata.l2_redirect = 1w1;
        meta.l2_metadata.l2_nexthop = ecmp_index;
        meta.l2_metadata.l2_nexthop_type = 1w1;
    }
    @name(".dmac_drop") action _dmac_drop() {
        mark_to_drop();
    }
    @name(".smac_miss") action _smac_miss() {
        meta.l2_metadata.l2_src_miss = 1w1;
    }
    @name(".smac_hit") action _smac_hit(bit<16> ifindex) {
        meta.l2_metadata.l2_src_move = meta.ingress_metadata.ifindex ^ ifindex;
    }
    @name(".dmac") table _dmac_0 {
        support_timeout = true;
        actions = {
            _nop_24();
            _dmac_hit();
            _dmac_multicast_hit();
            _dmac_miss();
            _dmac_redirect_nexthop();
            _dmac_redirect_ecmp();
            _dmac_drop();
            @defaultonly NoAction_133();
        }
        key = {
            meta.ingress_metadata.bd   : exact @name("ingress_metadata.bd") ;
            meta.l2_metadata.lkp_mac_da: exact @name("l2_metadata.lkp_mac_da") ;
        }
        size = 1024;
        default_action = NoAction_133();
    }
    @name(".smac") table _smac_0 {
        actions = {
            _nop_25();
            _smac_miss();
            _smac_hit();
            @defaultonly NoAction_134();
        }
        key = {
            meta.ingress_metadata.bd   : exact @name("ingress_metadata.bd") ;
            meta.l2_metadata.lkp_mac_sa: exact @name("l2_metadata.lkp_mac_sa") ;
        }
        size = 1024;
        default_action = NoAction_134();
    }
    @name(".nop") action _nop_26() {
    }
    @name(".acl_log") action _acl_log(bit<16> acl_stats_index) {
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_deny") action _acl_deny(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_deny = 1w1;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_permit") action _acl_permit(bit<16> acl_stats_index) {
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_mirror") action _acl_mirror(bit<32> session_id, bit<16> acl_stats_index) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.enable_dod = 1w0;
        clone3<tuple_0>(CloneType.I2E, session_id, { (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp, (bit<16>)session_id });
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".mac_acl") table _mac_acl_0 {
        actions = {
            _nop_26();
            _acl_log();
            _acl_deny();
            _acl_permit();
            _acl_mirror();
            @defaultonly NoAction_135();
        }
        key = {
            meta.acl_metadata.if_label   : ternary @name("acl_metadata.if_label") ;
            meta.acl_metadata.bd_label   : ternary @name("acl_metadata.bd_label") ;
            meta.l2_metadata.lkp_mac_sa  : ternary @name("l2_metadata.lkp_mac_sa") ;
            meta.l2_metadata.lkp_mac_da  : ternary @name("l2_metadata.lkp_mac_da") ;
            meta.l2_metadata.lkp_mac_type: ternary @name("l2_metadata.lkp_mac_type") ;
        }
        size = 512;
        default_action = NoAction_135();
    }
    @name(".nop") action _nop_27() {
    }
    @name(".nop") action _nop_28() {
    }
    @name(".acl_log") action _acl_log_0(bit<16> acl_stats_index) {
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_log") action _acl_log_4(bit<16> acl_stats_index) {
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_deny") action _acl_deny_0(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_deny = 1w1;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_deny") action _acl_deny_4(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_deny = 1w1;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_permit") action _acl_permit_0(bit<16> acl_stats_index) {
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_permit") action _acl_permit_4(bit<16> acl_stats_index) {
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_mirror") action _acl_mirror_0(bit<32> session_id, bit<16> acl_stats_index) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.enable_dod = 1w0;
        clone3<tuple_0>(CloneType.I2E, session_id, { (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp, (bit<16>)session_id });
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_mirror") action _acl_mirror_4(bit<32> session_id, bit<16> acl_stats_index) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.enable_dod = 1w0;
        clone3<tuple_0>(CloneType.I2E, session_id, { (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp, (bit<16>)session_id });
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_dod_en") action _acl_dod_en() {
        meta.ingress_metadata.enable_dod = 1w1;
    }
    @name(".acl_redirect_nexthop") action _acl_redirect_nexthop(bit<16> nexthop_index, bit<16> acl_stats_index) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = nexthop_index;
        meta.acl_metadata.acl_nexthop_type = 1w0;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_redirect_nexthop") action _acl_redirect_nexthop_2(bit<16> nexthop_index, bit<16> acl_stats_index) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = nexthop_index;
        meta.acl_metadata.acl_nexthop_type = 1w0;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_redirect_ecmp") action _acl_redirect_ecmp(bit<16> ecmp_index, bit<16> acl_stats_index) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = ecmp_index;
        meta.acl_metadata.acl_nexthop_type = 1w1;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_redirect_ecmp") action _acl_redirect_ecmp_2(bit<16> ecmp_index, bit<16> acl_stats_index) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = ecmp_index;
        meta.acl_metadata.acl_nexthop_type = 1w1;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".ip_acl") table _ip_acl_0 {
        actions = {
            _nop_27();
            _acl_log_0();
            _acl_deny_0();
            _acl_permit_0();
            _acl_mirror_0();
            _acl_dod_en();
            _acl_redirect_nexthop();
            _acl_redirect_ecmp();
            @defaultonly NoAction_136();
        }
        key = {
            meta.acl_metadata.if_label    : ternary @name("acl_metadata.if_label") ;
            meta.acl_metadata.bd_label    : ternary @name("acl_metadata.bd_label") ;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("ipv4_metadata.lkp_ipv4_sa") ;
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da") ;
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_sport : ternary @name("l3_metadata.lkp_l4_sport") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport") ;
            hdr.tcp.flags                 : ternary @name("tcp.flags") ;
            meta.l3_metadata.lkp_ip_ttl   : ternary @name("l3_metadata.lkp_ip_ttl") ;
        }
        size = 512;
        default_action = NoAction_136();
    }
    @name(".ipv6_acl") table _ipv6_acl_0 {
        actions = {
            _nop_28();
            _acl_log_4();
            _acl_deny_4();
            _acl_permit_4();
            _acl_mirror_4();
            _acl_redirect_nexthop_2();
            _acl_redirect_ecmp_2();
            @defaultonly NoAction_137();
        }
        key = {
            meta.acl_metadata.if_label    : ternary @name("acl_metadata.if_label") ;
            meta.acl_metadata.bd_label    : ternary @name("acl_metadata.bd_label") ;
            meta.ipv6_metadata.lkp_ipv6_sa: ternary @name("ipv6_metadata.lkp_ipv6_sa") ;
            meta.ipv6_metadata.lkp_ipv6_da: ternary @name("ipv6_metadata.lkp_ipv6_da") ;
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_sport : ternary @name("l3_metadata.lkp_l4_sport") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport") ;
            hdr.tcp.flags                 : ternary @name("tcp.flags") ;
            meta.l3_metadata.lkp_ip_ttl   : ternary @name("l3_metadata.lkp_ip_ttl") ;
        }
        size = 512;
        default_action = NoAction_137();
    }
    @name(".nop") action _nop_68() {
    }
    @name(".apply_cos_marking") action _apply_cos_marking(bit<3> cos) {
        meta.qos_metadata.marked_cos = cos;
    }
    @name(".apply_dscp_marking") action _apply_dscp_marking(bit<8> dscp) {
        meta.qos_metadata.marked_dscp = dscp;
    }
    @name(".apply_tc_marking") action _apply_tc_marking(bit<3> tc) {
        meta.qos_metadata.marked_exp = tc;
    }
    @name(".qos") table _qos_0 {
        actions = {
            _nop_68();
            _apply_cos_marking();
            _apply_dscp_marking();
            _apply_tc_marking();
            @defaultonly NoAction_138();
        }
        key = {
            meta.acl_metadata.if_label    : ternary @name("acl_metadata.if_label") ;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("ipv4_metadata.lkp_ipv4_sa") ;
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da") ;
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_ip_tc    : ternary @name("l3_metadata.lkp_ip_tc") ;
            meta.tunnel_metadata.mpls_exp : ternary @name("tunnel_metadata.mpls_exp") ;
            meta.qos_metadata.outer_dscp  : ternary @name("qos_metadata.outer_dscp") ;
        }
        size = 512;
        default_action = NoAction_138();
    }
    @name(".nop") action _nop_69() {
    }
    @name(".racl_log") action _racl_log(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_deny") action _racl_deny(bit<16> acl_stats_index) {
        meta.acl_metadata.racl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_permit") action _racl_permit(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_redirect_nexthop") action _racl_redirect_nexthop(bit<16> nexthop_index, bit<16> acl_stats_index) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = nexthop_index;
        meta.acl_metadata.racl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_redirect_ecmp") action _racl_redirect_ecmp(bit<16> ecmp_index, bit<16> acl_stats_index) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = ecmp_index;
        meta.acl_metadata.racl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".ipv4_racl") table _ipv4_racl_0 {
        actions = {
            _nop_69();
            _racl_log();
            _racl_deny();
            _racl_permit();
            _racl_redirect_nexthop();
            _racl_redirect_ecmp();
            @defaultonly NoAction_139();
        }
        key = {
            meta.acl_metadata.bd_label    : ternary @name("acl_metadata.bd_label") ;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("ipv4_metadata.lkp_ipv4_sa") ;
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da") ;
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_sport : ternary @name("l3_metadata.lkp_l4_sport") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport") ;
        }
        size = 512;
        default_action = NoAction_139();
    }
    @name(".on_miss") action _on_miss_5() {
    }
    @name(".ipv4_urpf_hit") action _ipv4_urpf_hit(bit<16> urpf_bd_group) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group;
        meta.l3_metadata.urpf_mode = meta.ipv4_metadata.ipv4_urpf_mode;
    }
    @name(".ipv4_urpf_hit") action _ipv4_urpf_hit_2(bit<16> urpf_bd_group) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group;
        meta.l3_metadata.urpf_mode = meta.ipv4_metadata.ipv4_urpf_mode;
    }
    @name(".urpf_miss") action _urpf_miss() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".ipv4_urpf") table _ipv4_urpf_0 {
        actions = {
            _on_miss_5();
            _ipv4_urpf_hit();
            @defaultonly NoAction_140();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("ipv4_metadata.lkp_ipv4_sa") ;
        }
        size = 1024;
        default_action = NoAction_140();
    }
    @name(".ipv4_urpf_lpm") table _ipv4_urpf_lpm_0 {
        actions = {
            _ipv4_urpf_hit_2();
            _urpf_miss();
            @defaultonly NoAction_141();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_sa: lpm @name("ipv4_metadata.lkp_ipv4_sa") ;
        }
        size = 512;
        default_action = NoAction_141();
    }
    @name(".on_miss") action _on_miss_6() {
    }
    @name(".on_miss") action _on_miss_7() {
    }
    @name(".fib_hit_nexthop") action _fib_hit_nexthop(bit<16> nexthop_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_nexthop") action _fib_hit_nexthop_0(bit<16> nexthop_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_ecmp") action _fib_hit_ecmp(bit<16> ecmp_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".fib_hit_ecmp") action _fib_hit_ecmp_0(bit<16> ecmp_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".ipv4_fib") table _ipv4_fib_0 {
        actions = {
            _on_miss_6();
            _fib_hit_nexthop();
            _fib_hit_ecmp();
            @defaultonly NoAction_142();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_da: exact @name("ipv4_metadata.lkp_ipv4_da") ;
        }
        size = 1024;
        default_action = NoAction_142();
    }
    @name(".ipv4_fib_lpm") table _ipv4_fib_lpm_0 {
        actions = {
            _on_miss_7();
            _fib_hit_nexthop_0();
            _fib_hit_ecmp_0();
            @defaultonly NoAction_143();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_da: lpm @name("ipv4_metadata.lkp_ipv4_da") ;
        }
        size = 512;
        default_action = NoAction_143();
    }
    @name(".nop") action _nop_70() {
    }
    @name(".racl_log") action _racl_log_0(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_deny") action _racl_deny_0(bit<16> acl_stats_index) {
        meta.acl_metadata.racl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_permit") action _racl_permit_0(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_redirect_nexthop") action _racl_redirect_nexthop_0(bit<16> nexthop_index, bit<16> acl_stats_index) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = nexthop_index;
        meta.acl_metadata.racl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_redirect_ecmp") action _racl_redirect_ecmp_0(bit<16> ecmp_index, bit<16> acl_stats_index) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = ecmp_index;
        meta.acl_metadata.racl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".ipv6_racl") table _ipv6_racl_0 {
        actions = {
            _nop_70();
            _racl_log_0();
            _racl_deny_0();
            _racl_permit_0();
            _racl_redirect_nexthop_0();
            _racl_redirect_ecmp_0();
            @defaultonly NoAction_144();
        }
        key = {
            meta.acl_metadata.bd_label    : ternary @name("acl_metadata.bd_label") ;
            meta.ipv6_metadata.lkp_ipv6_sa: ternary @name("ipv6_metadata.lkp_ipv6_sa") ;
            meta.ipv6_metadata.lkp_ipv6_da: ternary @name("ipv6_metadata.lkp_ipv6_da") ;
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_sport : ternary @name("l3_metadata.lkp_l4_sport") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport") ;
        }
        size = 512;
        default_action = NoAction_144();
    }
    @name(".on_miss") action _on_miss_8() {
    }
    @name(".ipv6_urpf_hit") action _ipv6_urpf_hit(bit<16> urpf_bd_group) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group;
        meta.l3_metadata.urpf_mode = meta.ipv6_metadata.ipv6_urpf_mode;
    }
    @name(".ipv6_urpf_hit") action _ipv6_urpf_hit_2(bit<16> urpf_bd_group) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group;
        meta.l3_metadata.urpf_mode = meta.ipv6_metadata.ipv6_urpf_mode;
    }
    @name(".urpf_miss") action _urpf_miss_0() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".ipv6_urpf") table _ipv6_urpf_0 {
        actions = {
            _on_miss_8();
            _ipv6_urpf_hit();
            @defaultonly NoAction_145();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_sa: exact @name("ipv6_metadata.lkp_ipv6_sa") ;
        }
        size = 1024;
        default_action = NoAction_145();
    }
    @name(".ipv6_urpf_lpm") table _ipv6_urpf_lpm_0 {
        actions = {
            _ipv6_urpf_hit_2();
            _urpf_miss_0();
            @defaultonly NoAction_146();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_sa: lpm @name("ipv6_metadata.lkp_ipv6_sa") ;
        }
        size = 512;
        default_action = NoAction_146();
    }
    @name(".on_miss") action _on_miss_17() {
    }
    @name(".on_miss") action _on_miss_18() {
    }
    @name(".fib_hit_nexthop") action _fib_hit_nexthop_5(bit<16> nexthop_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_nexthop") action _fib_hit_nexthop_6(bit<16> nexthop_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_ecmp") action _fib_hit_ecmp_5(bit<16> ecmp_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".fib_hit_ecmp") action _fib_hit_ecmp_6(bit<16> ecmp_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".ipv6_fib") table _ipv6_fib_0 {
        actions = {
            _on_miss_17();
            _fib_hit_nexthop_5();
            _fib_hit_ecmp_5();
            @defaultonly NoAction_147();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_da: exact @name("ipv6_metadata.lkp_ipv6_da") ;
        }
        size = 1024;
        default_action = NoAction_147();
    }
    @name(".ipv6_fib_lpm") table _ipv6_fib_lpm_0 {
        actions = {
            _on_miss_18();
            _fib_hit_nexthop_6();
            _fib_hit_ecmp_6();
            @defaultonly NoAction_148();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_da: lpm @name("ipv6_metadata.lkp_ipv6_da") ;
        }
        size = 512;
        default_action = NoAction_148();
    }
    @name(".nop") action _nop_71() {
    }
    @name(".urpf_bd_miss") action _urpf_bd_miss() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".urpf_bd") table _urpf_bd_0 {
        actions = {
            _nop_71();
            _urpf_bd_miss();
            @defaultonly NoAction_149();
        }
        key = {
            meta.l3_metadata.urpf_bd_group: exact @name("l3_metadata.urpf_bd_group") ;
            meta.ingress_metadata.bd      : exact @name("ingress_metadata.bd") ;
        }
        size = 1024;
        default_action = NoAction_149();
    }
    @name(".nop") action _nop_72() {
    }
    @name(".nop") action _nop_73() {
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
        meta.intrinsic_metadata.mcast_grp = hdr.fabric_header.dstPortOrGroup;
    }
    @name(".terminate_fabric_multicast_packet") action _terminate_fabric_multicast_packet() {
        meta.tunnel_metadata.tunnel_terminate = hdr.fabric_header_multicast.tunnelTerminate;
        meta.tunnel_metadata.ingress_tunnel_type = hdr.fabric_header_multicast.ingressTunnelType;
        meta.l3_metadata.nexthop_index = 16w0;
        meta.l3_metadata.routed = hdr.fabric_header_multicast.routed;
        meta.l3_metadata.outer_routed = hdr.fabric_header_multicast.outerRouted;
        meta.intrinsic_metadata.mcast_grp = hdr.fabric_header_multicast.mcastGrp;
        hdr.ethernet.etherType = hdr.fabric_payload_header.etherType;
        hdr.fabric_header.setInvalid();
        hdr.fabric_header_multicast.setInvalid();
        hdr.fabric_payload_header.setInvalid();
    }
    @name(".set_ingress_ifindex_properties") action _set_ingress_ifindex_properties() {
    }
    @name(".terminate_inner_ethernet_non_ip_over_fabric") action _terminate_inner_ethernet_non_ip_over_fabric() {
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_inner_ethernet_ipv4_over_fabric") action _terminate_inner_ethernet_ipv4_over_fabric() {
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.inner_ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.inner_ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv4.protocol;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
    }
    @name(".terminate_inner_ipv4_over_fabric") action _terminate_inner_ipv4_over_fabric() {
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.inner_ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.inner_ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv4.protocol;
        meta.l3_metadata.lkp_ip_ttl = hdr.inner_ipv4.ttl;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
    }
    @name(".terminate_inner_ethernet_ipv6_over_fabric") action _terminate_inner_ethernet_ipv6_over_fabric() {
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv6.nextHdr;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
    }
    @name(".terminate_inner_ipv6_over_fabric") action _terminate_inner_ipv6_over_fabric() {
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv6.nextHdr;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
    }
    @name(".fabric_ingress_dst_lkp") table _fabric_ingress_dst_lkp_0 {
        actions = {
            _nop_72();
            _terminate_cpu_packet();
            _switch_fabric_unicast_packet();
            _terminate_fabric_unicast_packet();
            _switch_fabric_multicast_packet();
            _terminate_fabric_multicast_packet();
            @defaultonly NoAction_150();
        }
        key = {
            hdr.fabric_header.dstDevice: exact @name("fabric_header.dstDevice") ;
        }
        default_action = NoAction_150();
    }
    @name(".fabric_ingress_src_lkp") table _fabric_ingress_src_lkp_0 {
        actions = {
            _nop_73();
            _set_ingress_ifindex_properties();
            @defaultonly NoAction_151();
        }
        key = {
            hdr.fabric_header_multicast.ingressIfindex: exact @name("fabric_header_multicast.ingressIfindex") ;
        }
        size = 1024;
        default_action = NoAction_151();
    }
    @name(".tunneled_packet_over_fabric") table _tunneled_packet_over_fabric_0 {
        actions = {
            _terminate_inner_ethernet_non_ip_over_fabric();
            _terminate_inner_ethernet_ipv4_over_fabric();
            _terminate_inner_ipv4_over_fabric();
            _terminate_inner_ethernet_ipv6_over_fabric();
            _terminate_inner_ipv6_over_fabric();
            @defaultonly NoAction_152();
        }
        key = {
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type") ;
            hdr.inner_ipv4.isValid()                : exact @name("inner_ipv4.$valid$") ;
            hdr.inner_ipv6.isValid()                : exact @name("inner_ipv6.$valid$") ;
        }
        size = 1024;
        default_action = NoAction_152();
    }
    @name(".compute_lkp_ipv4_hash") action _compute_lkp_ipv4_hash() {
        hash<bit<16>, bit<16>, tuple_2, bit<32>>(meta.hash_metadata.hash1, HashAlgorithm.crc16, 16w0, { meta.ipv4_metadata.lkp_ipv4_sa, meta.ipv4_metadata.lkp_ipv4_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
        hash<bit<16>, bit<16>, tuple_3, bit<32>>(meta.hash_metadata.hash2, HashAlgorithm.crc16, 16w0, { meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.ipv4_metadata.lkp_ipv4_sa, meta.ipv4_metadata.lkp_ipv4_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
    }
    @name(".compute_lkp_ipv6_hash") action _compute_lkp_ipv6_hash() {
        hash<bit<16>, bit<16>, tuple_4, bit<32>>(meta.hash_metadata.hash1, HashAlgorithm.crc16, 16w0, { meta.ipv6_metadata.lkp_ipv6_sa, meta.ipv6_metadata.lkp_ipv6_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
        hash<bit<16>, bit<16>, tuple_5, bit<32>>(meta.hash_metadata.hash2, HashAlgorithm.crc16, 16w0, { meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.ipv6_metadata.lkp_ipv6_sa, meta.ipv6_metadata.lkp_ipv6_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
    }
    @name(".compute_lkp_non_ip_hash") action _compute_lkp_non_ip_hash() {
        hash<bit<16>, bit<16>, tuple_6, bit<32>>(meta.hash_metadata.hash2, HashAlgorithm.crc16, 16w0, { meta.ingress_metadata.ifindex, meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.l2_metadata.lkp_mac_type }, 32w65536);
    }
    @name(".computed_two_hashes") action _computed_two_hashes() {
        meta.intrinsic_metadata.mcast_hash = (bit<13>)meta.hash_metadata.hash1;
        meta.hash_metadata.entropy_hash = meta.hash_metadata.hash2;
    }
    @name(".computed_one_hash") action _computed_one_hash() {
        meta.hash_metadata.hash1 = meta.hash_metadata.hash2;
        meta.intrinsic_metadata.mcast_hash = (bit<13>)meta.hash_metadata.hash2;
        meta.hash_metadata.entropy_hash = meta.hash_metadata.hash2;
    }
    @name(".compute_ipv4_hashes") table _compute_ipv4_hashes_0 {
        actions = {
            _compute_lkp_ipv4_hash();
            @defaultonly NoAction_153();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("ingress_metadata.drop_flag") ;
        }
        default_action = NoAction_153();
    }
    @name(".compute_ipv6_hashes") table _compute_ipv6_hashes_0 {
        actions = {
            _compute_lkp_ipv6_hash();
            @defaultonly NoAction_154();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("ingress_metadata.drop_flag") ;
        }
        default_action = NoAction_154();
    }
    @name(".compute_non_ip_hashes") table _compute_non_ip_hashes_0 {
        actions = {
            _compute_lkp_non_ip_hash();
            @defaultonly NoAction_155();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("ingress_metadata.drop_flag") ;
        }
        default_action = NoAction_155();
    }
    @name(".compute_other_hashes") table _compute_other_hashes_0 {
        actions = {
            _computed_two_hashes();
            _computed_one_hash();
            @defaultonly NoAction_156();
        }
        key = {
            meta.hash_metadata.hash1: exact @name("hash_metadata.hash1") ;
        }
        default_action = NoAction_156();
    }
    @min_width(32) @name(".ingress_bd_stats_count") counter(32w1024, CounterType.packets_and_bytes) _ingress_bd_stats_count_0;
    @name(".update_ingress_bd_stats") action _update_ingress_bd_stats() {
        _ingress_bd_stats_count_0.count((bit<32>)meta.l2_metadata.bd_stats_idx);
    }
    @name(".ingress_bd_stats") table _ingress_bd_stats_0 {
        actions = {
            _update_ingress_bd_stats();
            @defaultonly NoAction_157();
        }
        size = 1024;
        default_action = NoAction_157();
    }
    @name(".acl_stats_count") counter(32w1024, CounterType.packets_and_bytes) _acl_stats_count_0;
    @name(".acl_stats_update") action _acl_stats_update() {
        _acl_stats_count_0.count((bit<32>)meta.acl_metadata.acl_stats_index);
    }
    @name(".acl_stats") table _acl_stats_0 {
        actions = {
            _acl_stats_update();
            @defaultonly NoAction_158();
        }
        key = {
            meta.acl_metadata.acl_stats_index: exact @name("acl_metadata.acl_stats_index") ;
        }
        size = 1024;
        default_action = NoAction_158();
    }
    @name(".nop") action _nop_74() {
    }
    @name(".set_l2_redirect_action") action _set_l2_redirect_action() {
        meta.l3_metadata.nexthop_index = meta.l2_metadata.l2_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.l2_metadata.l2_nexthop_type;
    }
    @name(".set_fib_redirect_action") action _set_fib_redirect_action() {
        meta.l3_metadata.nexthop_index = meta.l3_metadata.fib_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.l3_metadata.fib_nexthop_type;
        meta.l3_metadata.routed = 1w1;
        meta.intrinsic_metadata.mcast_grp = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_cpu_redirect_action") action _set_cpu_redirect_action() {
        meta.l3_metadata.routed = 1w0;
        meta.intrinsic_metadata.mcast_grp = 16w0;
        standard_metadata.egress_spec = 9w64;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_acl_redirect_action") action _set_acl_redirect_action() {
        meta.l3_metadata.nexthop_index = meta.acl_metadata.acl_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.acl_metadata.acl_nexthop_type;
    }
    @name(".set_racl_redirect_action") action _set_racl_redirect_action() {
        meta.l3_metadata.nexthop_index = meta.acl_metadata.racl_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.acl_metadata.racl_nexthop_type;
        meta.l3_metadata.routed = 1w1;
    }
    @name(".fwd_result") table _fwd_result_0 {
        actions = {
            _nop_74();
            _set_l2_redirect_action();
            _set_fib_redirect_action();
            _set_cpu_redirect_action();
            _set_acl_redirect_action();
            _set_racl_redirect_action();
            @defaultonly NoAction_159();
        }
        key = {
            meta.l2_metadata.l2_redirect   : ternary @name("l2_metadata.l2_redirect") ;
            meta.acl_metadata.acl_redirect : ternary @name("acl_metadata.acl_redirect") ;
            meta.acl_metadata.racl_redirect: ternary @name("acl_metadata.racl_redirect") ;
            meta.l3_metadata.rmac_hit      : ternary @name("l3_metadata.rmac_hit") ;
            meta.l3_metadata.fib_hit       : ternary @name("l3_metadata.fib_hit") ;
        }
        size = 512;
        default_action = NoAction_159();
    }
    @name(".nop") action _nop_75() {
    }
    @name(".nop") action _nop_76() {
    }
    @name(".set_ecmp_nexthop_details") action _set_ecmp_nexthop_details(bit<16> ifindex, bit<16> bd, bit<16> nhop_index, bit<1> tunnel) {
        meta.ingress_metadata.egress_ifindex = ifindex;
        meta.l3_metadata.nexthop_index = nhop_index;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex;
        meta.tunnel_metadata.tunnel_if_check = meta.tunnel_metadata.tunnel_terminate ^ tunnel;
    }
    @name(".set_ecmp_nexthop_details_for_post_routed_flood") action _set_ecmp_nexthop_details_for_post_routed_flood(bit<16> bd, bit<16> uuc_mc_index, bit<16> nhop_index) {
        meta.intrinsic_metadata.mcast_grp = uuc_mc_index;
        meta.l3_metadata.nexthop_index = nhop_index;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".set_nexthop_details") action _set_nexthop_details(bit<16> ifindex, bit<16> bd, bit<1> tunnel) {
        meta.ingress_metadata.egress_ifindex = ifindex;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex;
        meta.tunnel_metadata.tunnel_if_check = meta.tunnel_metadata.tunnel_terminate ^ tunnel;
    }
    @name(".set_nexthop_details_for_post_routed_flood") action _set_nexthop_details_for_post_routed_flood(bit<16> bd, bit<16> uuc_mc_index) {
        meta.intrinsic_metadata.mcast_grp = uuc_mc_index;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".ecmp_group") table _ecmp_group_0 {
        actions = {
            _nop_75();
            _set_ecmp_nexthop_details();
            _set_ecmp_nexthop_details_for_post_routed_flood();
            @defaultonly NoAction_160();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("l3_metadata.nexthop_index") ;
            meta.hash_metadata.hash1      : selector @name("hash_metadata.hash1") ;
        }
        size = 1024;
        implementation = ecmp_action_profile;
        default_action = NoAction_160();
    }
    @name(".nexthop") table _nexthop_0 {
        actions = {
            _nop_76();
            _set_nexthop_details();
            _set_nexthop_details_for_post_routed_flood();
            @defaultonly NoAction_161();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("l3_metadata.nexthop_index") ;
        }
        size = 1024;
        default_action = NoAction_161();
    }
    @name(".nop") action _nop_77() {
    }
    @name(".set_bd_flood_mc_index") action _set_bd_flood_mc_index(bit<16> mc_index) {
        meta.intrinsic_metadata.mcast_grp = mc_index;
    }
    @name(".bd_flood") table _bd_flood_0 {
        actions = {
            _nop_77();
            _set_bd_flood_mc_index();
            @defaultonly NoAction_162();
        }
        key = {
            meta.ingress_metadata.bd     : exact @name("ingress_metadata.bd") ;
            meta.l2_metadata.lkp_pkt_type: exact @name("l2_metadata.lkp_pkt_type") ;
        }
        size = 1024;
        default_action = NoAction_162();
    }
    @name(".set_lag_miss") action _set_lag_miss() {
    }
    @name(".set_lag_port") action _set_lag_port(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".set_lag_remote_port") action _set_lag_remote_port(bit<8> device, bit<16> port) {
        meta.fabric_metadata.dst_device = device;
        meta.fabric_metadata.dst_port = port;
    }
    @name(".lag_group") table _lag_group_0 {
        actions = {
            _set_lag_miss();
            _set_lag_port();
            _set_lag_remote_port();
            @defaultonly NoAction_163();
        }
        key = {
            meta.ingress_metadata.egress_ifindex: exact @name("ingress_metadata.egress_ifindex") ;
            meta.hash_metadata.hash2            : selector @name("hash_metadata.hash2") ;
        }
        size = 1024;
        implementation = lag_action_profile;
        default_action = NoAction_163();
    }
    @name(".nop") action _nop_78() {
    }
    @name(".generate_learn_notify") action _generate_learn_notify() {
        digest<mac_learn_digest>(32w1024, { meta.ingress_metadata.bd, meta.l2_metadata.lkp_mac_sa, meta.ingress_metadata.ifindex });
    }
    @name(".learn_notify") table _learn_notify_0 {
        actions = {
            _nop_78();
            _generate_learn_notify();
            @defaultonly NoAction_164();
        }
        key = {
            meta.l2_metadata.l2_src_miss: ternary @name("l2_metadata.l2_src_miss") ;
            meta.l2_metadata.l2_src_move: ternary @name("l2_metadata.l2_src_move") ;
            meta.l2_metadata.stp_state  : ternary @name("l2_metadata.stp_state") ;
        }
        size = 512;
        default_action = NoAction_164();
    }
    @name(".nop") action _nop_79() {
    }
    @name(".set_fabric_lag_port") action _set_fabric_lag_port(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".set_fabric_multicast") action _set_fabric_multicast(bit<8> fabric_mgid) {
        meta.multicast_metadata.mcast_grp = meta.intrinsic_metadata.mcast_grp;
    }
    @name(".fabric_lag") table _fabric_lag_0 {
        actions = {
            _nop_79();
            _set_fabric_lag_port();
            _set_fabric_multicast();
            @defaultonly NoAction_165();
        }
        key = {
            meta.fabric_metadata.dst_device: exact @name("fabric_metadata.dst_device") ;
            meta.hash_metadata.hash2       : selector @name("hash_metadata.hash2") ;
        }
        implementation = fabric_lag_action_profile;
        default_action = NoAction_165();
    }
    @name(".drop_stats") counter(32w1024, CounterType.packets) _drop_stats_2;
    @name(".drop_stats_2") counter(32w1024, CounterType.packets) _drop_stats_3;
    @name(".drop_stats_update") action _drop_stats_update() {
        _drop_stats_3.count((bit<32>)meta.ingress_metadata.drop_reason);
    }
    @name(".nop") action _nop_80() {
    }
    @name(".copy_to_cpu") action _copy_to_cpu(bit<16> reason_code) {
        meta.fabric_metadata.reason_code = reason_code;
        clone3<tuple_1>(CloneType.I2E, 32w250, { meta.ingress_metadata.bd, meta.ingress_metadata.ifindex, reason_code, meta.ingress_metadata.ingress_port });
    }
    @name(".redirect_to_cpu") action _redirect_to_cpu(bit<16> reason_code) {
        meta.fabric_metadata.reason_code = reason_code;
        clone3<tuple_1>(CloneType.I2E, 32w250, { meta.ingress_metadata.bd, meta.ingress_metadata.ifindex, reason_code, meta.ingress_metadata.ingress_port });
        mark_to_drop();
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".drop_packet") action _drop_packet() {
        mark_to_drop();
    }
    @name(".drop_packet_with_reason") action _drop_packet_with_reason(bit<32> drop_reason) {
        _drop_stats_2.count(drop_reason);
        mark_to_drop();
    }
    @name(".negative_mirror") action _negative_mirror(bit<32> session_id) {
        clone3<tuple_7>(CloneType.I2E, session_id, { meta.ingress_metadata.ifindex, meta.ingress_metadata.drop_reason });
        mark_to_drop();
    }
    @name(".congestion_mirror_set") action _congestion_mirror_set() {
        meta.intrinsic_metadata.deflect_on_drop = 1w1;
    }
    @name(".drop_stats") table _drop_stats_4 {
        actions = {
            _drop_stats_update();
            @defaultonly NoAction_166();
        }
        size = 1024;
        default_action = NoAction_166();
    }
    @name(".system_acl") table _system_acl_0 {
        actions = {
            _nop_80();
            _redirect_to_cpu();
            _copy_to_cpu();
            _drop_packet();
            _drop_packet_with_reason();
            _negative_mirror();
            _congestion_mirror_set();
            @defaultonly NoAction_167();
        }
        key = {
            meta.acl_metadata.if_label               : ternary @name("acl_metadata.if_label") ;
            meta.acl_metadata.bd_label               : ternary @name("acl_metadata.bd_label") ;
            meta.ipv4_metadata.lkp_ipv4_sa           : ternary @name("ipv4_metadata.lkp_ipv4_sa") ;
            meta.ipv4_metadata.lkp_ipv4_da           : ternary @name("ipv4_metadata.lkp_ipv4_da") ;
            meta.l3_metadata.lkp_ip_proto            : ternary @name("l3_metadata.lkp_ip_proto") ;
            meta.l2_metadata.lkp_mac_sa              : ternary @name("l2_metadata.lkp_mac_sa") ;
            meta.l2_metadata.lkp_mac_da              : ternary @name("l2_metadata.lkp_mac_da") ;
            meta.l2_metadata.lkp_mac_type            : ternary @name("l2_metadata.lkp_mac_type") ;
            meta.ingress_metadata.ifindex            : ternary @name("ingress_metadata.ifindex") ;
            meta.l2_metadata.port_vlan_mapping_miss  : ternary @name("l2_metadata.port_vlan_mapping_miss") ;
            meta.security_metadata.ipsg_check_fail   : ternary @name("security_metadata.ipsg_check_fail") ;
            meta.acl_metadata.acl_deny               : ternary @name("acl_metadata.acl_deny") ;
            meta.acl_metadata.racl_deny              : ternary @name("acl_metadata.racl_deny") ;
            meta.l3_metadata.urpf_check_fail         : ternary @name("l3_metadata.urpf_check_fail") ;
            meta.ingress_metadata.drop_flag          : ternary @name("ingress_metadata.drop_flag") ;
            meta.l3_metadata.rmac_hit                : ternary @name("l3_metadata.rmac_hit") ;
            meta.l3_metadata.routed                  : ternary @name("l3_metadata.routed") ;
            meta.ipv6_metadata.ipv6_src_is_link_local: ternary @name("ipv6_metadata.ipv6_src_is_link_local") ;
            meta.l2_metadata.same_if_check           : ternary @name("l2_metadata.same_if_check") ;
            meta.tunnel_metadata.tunnel_if_check     : ternary @name("tunnel_metadata.tunnel_if_check") ;
            meta.l3_metadata.same_bd_check           : ternary @name("l3_metadata.same_bd_check") ;
            meta.l3_metadata.lkp_ip_ttl              : ternary @name("l3_metadata.lkp_ip_ttl") ;
            meta.l2_metadata.stp_state               : ternary @name("l2_metadata.stp_state") ;
            meta.ingress_metadata.control_frame      : ternary @name("ingress_metadata.control_frame") ;
            meta.ipv4_metadata.ipv4_unicast_enabled  : ternary @name("ipv4_metadata.ipv4_unicast_enabled") ;
            meta.ingress_metadata.egress_ifindex     : ternary @name("ingress_metadata.egress_ifindex") ;
            meta.ingress_metadata.enable_dod         : ternary @name("ingress_metadata.enable_dod") ;
        }
        size = 512;
        default_action = NoAction_167();
    }
    apply {
        _ingress_port_mapping_0.apply();
        switch (_validate_outer_ethernet_0.apply().action_run) {
            default: {
                if (hdr.ipv4.isValid()) 
                    _validate_outer_ipv4_packet.apply();
                else 
                    if (hdr.ipv6.isValid()) 
                        _validate_outer_ipv6_packet.apply();
                    else 
                        if (hdr.mpls[0].isValid()) 
                            _validate_mpls_packet.apply();
            }
            _malformed_outer_ethernet_packet: {
            }
        }

        if (meta.ingress_metadata.port_type == 2w0) {
            _storm_control_0.apply();
            _port_vlan_mapping_0.apply();
            if (meta.l2_metadata.stp_group != 10w0) 
                _spanning_tree_0.apply();
            if (meta.security_metadata.ipsg_enabled == 1w1) 
                switch (_ipsg_0.apply().action_run) {
                    _on_miss_1: {
                        _ipsg_permit_special_0.apply();
                    }
                }

            switch (_outer_rmac_0.apply().action_run) {
                _outer_rmac_hit: {
                    if (hdr.ipv4.isValid()) 
                        switch (_ipv4_src_vtep.apply().action_run) {
                            _src_vtep_hit_1: {
                                _ipv4_dest_vtep.apply();
                            }
                        }

                    else 
                        if (hdr.ipv6.isValid()) 
                            switch (_ipv6_src_vtep.apply().action_run) {
                                _src_vtep_hit_2: {
                                    _ipv6_dest_vtep.apply();
                                }
                            }

                        else 
                            if (hdr.mpls[0].isValid()) 
                                _mpls.apply();
                }
            }

            if (meta.tunnel_metadata.tunnel_terminate == 1w1) 
                _tunnel_0.apply();
            if (!hdr.mpls[0].isValid() || hdr.mpls[0].isValid() && meta.tunnel_metadata.tunnel_terminate == 1w1) {
                if (meta.ingress_metadata.drop_flag == 1w0) 
                    _validate_packet_0.apply();
                _smac_0.apply();
                _dmac_0.apply();
                if (meta.l3_metadata.lkp_ip_type == 2w0) 
                    _mac_acl_0.apply();
                else 
                    if (meta.l3_metadata.lkp_ip_type == 2w1) 
                        _ip_acl_0.apply();
                    else 
                        if (meta.l3_metadata.lkp_ip_type == 2w2) 
                            _ipv6_acl_0.apply();
                _qos_0.apply();
                switch (rmac.apply().action_run) {
                    rmac_hit_0: {
                        if (meta.l3_metadata.lkp_ip_type == 2w1 && meta.ipv4_metadata.ipv4_unicast_enabled == 1w1) {
                            _ipv4_racl_0.apply();
                            if (meta.ipv4_metadata.ipv4_urpf_mode != 2w0) 
                                switch (_ipv4_urpf_0.apply().action_run) {
                                    _on_miss_5: {
                                        _ipv4_urpf_lpm_0.apply();
                                    }
                                }

                            switch (_ipv4_fib_0.apply().action_run) {
                                _on_miss_6: {
                                    _ipv4_fib_lpm_0.apply();
                                }
                            }

                        }
                        else 
                            if (meta.l3_metadata.lkp_ip_type == 2w2 && meta.ipv6_metadata.ipv6_unicast_enabled == 1w1) {
                                _ipv6_racl_0.apply();
                                if (meta.ipv6_metadata.ipv6_urpf_mode != 2w0) 
                                    switch (_ipv6_urpf_0.apply().action_run) {
                                        _on_miss_8: {
                                            _ipv6_urpf_lpm_0.apply();
                                        }
                                    }

                                switch (_ipv6_fib_0.apply().action_run) {
                                    _on_miss_17: {
                                        _ipv6_fib_lpm_0.apply();
                                    }
                                }

                            }
                        if (meta.l3_metadata.urpf_mode == 2w2 && meta.l3_metadata.urpf_hit == 1w1) 
                            _urpf_bd_0.apply();
                    }
                }

            }
        }
        else {
            _fabric_ingress_dst_lkp_0.apply();
            if (hdr.fabric_header_multicast.isValid()) 
                _fabric_ingress_src_lkp_0.apply();
            if (meta.tunnel_metadata.tunnel_terminate == 1w1) 
                _tunneled_packet_over_fabric_0.apply();
        }
        if (meta.tunnel_metadata.tunnel_terminate == 1w0 && hdr.ipv4.isValid() || meta.tunnel_metadata.tunnel_terminate == 1w1 && hdr.inner_ipv4.isValid()) 
            _compute_ipv4_hashes_0.apply();
        else 
            if (meta.tunnel_metadata.tunnel_terminate == 1w0 && hdr.ipv6.isValid() || meta.tunnel_metadata.tunnel_terminate == 1w1 && hdr.inner_ipv6.isValid()) 
                _compute_ipv6_hashes_0.apply();
            else 
                _compute_non_ip_hashes_0.apply();
        _compute_other_hashes_0.apply();
        if (meta.ingress_metadata.port_type == 2w0) {
            _ingress_bd_stats_0.apply();
            _acl_stats_0.apply();
            _fwd_result_0.apply();
            if (meta.nexthop_metadata.nexthop_type == 1w1) 
                _ecmp_group_0.apply();
            else 
                _nexthop_0.apply();
            if (meta.ingress_metadata.egress_ifindex == 16w65535) 
                _bd_flood_0.apply();
            else 
                _lag_group_0.apply();
            if (meta.l2_metadata.learning_enabled == 1w1) 
                _learn_notify_0.apply();
        }
        _fabric_lag_0.apply();
        _system_acl_0.apply();
        if (meta.ingress_metadata.drop_flag == 1w1) 
            _drop_stats_4.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<fabric_header_t>(hdr.fabric_header);
        packet.emit<fabric_header_cpu_t>(hdr.fabric_header_cpu);
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
        packet.emit<genv_t>(hdr.genv);
        packet.emit<vxlan_t>(hdr.vxlan);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<icmp_t>(hdr.icmp);
        packet.emit<mpls_t>(hdr.mpls[0]);
        packet.emit<mpls_t>(hdr.mpls[1]);
        packet.emit<mpls_t>(hdr.mpls[2]);
        packet.emit<ethernet_t>(hdr.inner_ethernet);
        packet.emit<ipv6_t>(hdr.inner_ipv6);
        packet.emit<ipv4_t>(hdr.inner_ipv4);
        packet.emit<udp_t>(hdr.inner_udp);
        packet.emit<tcp_t>(hdr.inner_tcp);
        packet.emit<icmp_t>(hdr.inner_icmp);
    }
}

struct tuple_8 {
    bit<4>  field_35;
    bit<4>  field_36;
    bit<8>  field_37;
    bit<16> field_38;
    bit<16> field_39;
    bit<3>  field_40;
    bit<13> field_41;
    bit<8>  field_42;
    bit<8>  field_43;
    bit<32> field_44;
    bit<32> field_45;
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple_8, bit<16>>(hdr.inner_ipv4.ihl == 4w5, { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr }, hdr.inner_ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum<tuple_8, bit<16>>(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_8, bit<16>>(hdr.inner_ipv4.ihl == 4w5, { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr }, hdr.inner_ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum<tuple_8, bit<16>>(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

