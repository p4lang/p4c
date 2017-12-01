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
    bit<4> tmp;
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
        tmp = packet.lookahead<bit<4>>();
        transition select(tmp[3:0]) {
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

control process_replication(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_0() {
    }
    @name(".set_replica_copy_bridged") action set_replica_copy_bridged_0() {
        meta.egress_metadata.routed = 1w0;
    }
    @name(".outer_replica_from_rid") action outer_replica_from_rid_0(bit<16> bd, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.bd = bd;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w0;
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.same_bd_check = bd ^ meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".outer_replica_from_rid_with_nexthop") action outer_replica_from_rid_with_nexthop_0(bit<16> bd, bit<16> nexthop_index, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.bd = bd;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w0;
        meta.egress_metadata.routed = meta.l3_metadata.outer_routed;
        meta.l3_metadata.nexthop_index = nexthop_index;
        meta.egress_metadata.same_bd_check = bd ^ meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".inner_replica_from_rid") action inner_replica_from_rid_0(bit<16> bd, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.bd = bd;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w1;
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.same_bd_check = bd ^ meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".inner_replica_from_rid_with_nexthop") action inner_replica_from_rid_with_nexthop_0(bit<16> bd, bit<16> nexthop_index, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.bd = bd;
        meta.multicast_metadata.replica = 1w1;
        meta.multicast_metadata.inner_replica = 1w1;
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.l3_metadata.nexthop_index = nexthop_index;
        meta.egress_metadata.same_bd_check = bd ^ meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".replica_type") table replica_type_0 {
        actions = {
            nop_0();
            set_replica_copy_bridged_0();
            @defaultonly NoAction();
        }
        key = {
            meta.multicast_metadata.replica   : exact @name("multicast_metadata.replica") ;
            meta.egress_metadata.same_bd_check: ternary @name("egress_metadata.same_bd_check") ;
        }
        size = 512;
        default_action = NoAction();
    }
    @name(".rid") table rid_0 {
        actions = {
            nop_0();
            outer_replica_from_rid_0();
            outer_replica_from_rid_with_nexthop_0();
            inner_replica_from_rid_0();
            inner_replica_from_rid_with_nexthop_0();
            @defaultonly NoAction();
        }
        key = {
            meta.intrinsic_metadata.egress_rid: exact @name("intrinsic_metadata.egress_rid") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        if (meta.intrinsic_metadata.egress_rid != 16w0) {
            rid_0.apply();
            replica_type_0.apply();
        }
    }
}

control process_vlan_decap(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_1() {
    }
    @name(".remove_vlan_single_tagged") action remove_vlan_single_tagged_0() {
        hdr.ethernet.etherType = hdr.vlan_tag_[0].etherType;
        hdr.vlan_tag_[0].setInvalid();
    }
    @name(".remove_vlan_double_tagged") action remove_vlan_double_tagged_0() {
        hdr.ethernet.etherType = hdr.vlan_tag_[1].etherType;
        hdr.vlan_tag_[0].setInvalid();
        hdr.vlan_tag_[1].setInvalid();
    }
    @name(".vlan_decap") table vlan_decap_0 {
        actions = {
            nop_1();
            remove_vlan_single_tagged_0();
            remove_vlan_double_tagged_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.vlan_tag_[0].isValid(): exact @name("vlan_tag_[0].$valid$") ;
            hdr.vlan_tag_[1].isValid(): exact @name("vlan_tag_[1].$valid$") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        vlan_decap_0.apply();
    }
}

control process_tunnel_decap(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".decap_inner_udp") action decap_inner_udp_0() {
        hdr.udp = hdr.inner_udp;
        hdr.inner_udp.setInvalid();
    }
    @name(".decap_inner_tcp") action decap_inner_tcp_0() {
        hdr.tcp = hdr.inner_tcp;
        hdr.inner_tcp.setInvalid();
        hdr.udp.setInvalid();
    }
    @name(".decap_inner_icmp") action decap_inner_icmp_0() {
        hdr.icmp = hdr.inner_icmp;
        hdr.inner_icmp.setInvalid();
        hdr.udp.setInvalid();
    }
    @name(".decap_inner_unknown") action decap_inner_unknown_0() {
        hdr.udp.setInvalid();
    }
    @name(".decap_vxlan_inner_ipv4") action decap_vxlan_inner_ipv4_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.vxlan.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_vxlan_inner_ipv6") action decap_vxlan_inner_ipv6_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.vxlan.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_vxlan_inner_non_ip") action decap_vxlan_inner_non_ip_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.vxlan.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_genv_inner_ipv4") action decap_genv_inner_ipv4_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.genv.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_genv_inner_ipv6") action decap_genv_inner_ipv6_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.genv.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_genv_inner_non_ip") action decap_genv_inner_non_ip_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.genv.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_nvgre_inner_ipv4") action decap_nvgre_inner_ipv4_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_nvgre_inner_ipv6") action decap_nvgre_inner_ipv6_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_nvgre_inner_non_ip") action decap_nvgre_inner_non_ip_0() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.nvgre.setInvalid();
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".decap_ip_inner_ipv4") action decap_ip_inner_ipv4_0() {
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.gre.setInvalid();
        hdr.ipv6.setInvalid();
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_ip_inner_ipv6") action decap_ip_inner_ipv6_0() {
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.gre.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ipv4_pop1") action decap_mpls_inner_ipv4_pop1_0() {
        hdr.mpls[0].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop1") action decap_mpls_inner_ipv6_pop1_0() {
        hdr.mpls[0].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop1") action decap_mpls_inner_ethernet_ipv4_pop1_0() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop1") action decap_mpls_inner_ethernet_ipv6_pop1_0() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop1") action decap_mpls_inner_ethernet_non_ip_pop1_0() {
        hdr.mpls[0].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".decap_mpls_inner_ipv4_pop2") action decap_mpls_inner_ipv4_pop2_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop2") action decap_mpls_inner_ipv6_pop2_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop2") action decap_mpls_inner_ethernet_ipv4_pop2_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop2") action decap_mpls_inner_ethernet_ipv6_pop2_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop2") action decap_mpls_inner_ethernet_non_ip_pop2_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".decap_mpls_inner_ipv4_pop3") action decap_mpls_inner_ipv4_pop3_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".decap_mpls_inner_ipv6_pop3") action decap_mpls_inner_ipv6_pop3_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".decap_mpls_inner_ethernet_ipv4_pop3") action decap_mpls_inner_ethernet_ipv4_pop3_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv4.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_ipv6_pop3") action decap_mpls_inner_ethernet_ipv6_pop3_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ethernet.setInvalid();
        hdr.inner_ipv6.setInvalid();
    }
    @name(".decap_mpls_inner_ethernet_non_ip_pop3") action decap_mpls_inner_ethernet_non_ip_pop3_0() {
        hdr.mpls[0].setInvalid();
        hdr.mpls[1].setInvalid();
        hdr.mpls[2].setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
    }
    @name(".tunnel_decap_process_inner") table tunnel_decap_process_inner_0 {
        actions = {
            decap_inner_udp_0();
            decap_inner_tcp_0();
            decap_inner_icmp_0();
            decap_inner_unknown_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.inner_tcp.isValid() : exact @name("inner_tcp.$valid$") ;
            hdr.inner_udp.isValid() : exact @name("inner_udp.$valid$") ;
            hdr.inner_icmp.isValid(): exact @name("inner_icmp.$valid$") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunnel_decap_process_outer") table tunnel_decap_process_outer_0 {
        actions = {
            decap_vxlan_inner_ipv4_0();
            decap_vxlan_inner_ipv6_0();
            decap_vxlan_inner_non_ip_0();
            decap_genv_inner_ipv4_0();
            decap_genv_inner_ipv6_0();
            decap_genv_inner_non_ip_0();
            decap_nvgre_inner_ipv4_0();
            decap_nvgre_inner_ipv6_0();
            decap_nvgre_inner_non_ip_0();
            decap_ip_inner_ipv4_0();
            decap_ip_inner_ipv6_0();
            decap_mpls_inner_ipv4_pop1_0();
            decap_mpls_inner_ipv6_pop1_0();
            decap_mpls_inner_ethernet_ipv4_pop1_0();
            decap_mpls_inner_ethernet_ipv6_pop1_0();
            decap_mpls_inner_ethernet_non_ip_pop1_0();
            decap_mpls_inner_ipv4_pop2_0();
            decap_mpls_inner_ipv6_pop2_0();
            decap_mpls_inner_ethernet_ipv4_pop2_0();
            decap_mpls_inner_ethernet_ipv6_pop2_0();
            decap_mpls_inner_ethernet_non_ip_pop2_0();
            decap_mpls_inner_ipv4_pop3_0();
            decap_mpls_inner_ipv6_pop3_0();
            decap_mpls_inner_ethernet_ipv4_pop3_0();
            decap_mpls_inner_ethernet_ipv6_pop3_0();
            decap_mpls_inner_ethernet_non_ip_pop3_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type") ;
            hdr.inner_ipv4.isValid()                : exact @name("inner_ipv4.$valid$") ;
            hdr.inner_ipv6.isValid()                : exact @name("inner_ipv6.$valid$") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        if (meta.tunnel_metadata.tunnel_terminate == 1w1) 
            if (meta.multicast_metadata.inner_replica == 1w1 || meta.multicast_metadata.replica == 1w0) {
                tunnel_decap_process_outer_0.apply();
                tunnel_decap_process_inner_0.apply();
            }
    }
}

control process_egress_bd(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_2() {
    }
    @name(".set_egress_bd_properties") action set_egress_bd_properties_0() {
    }
    @name(".egress_bd_map") table egress_bd_map_0 {
        actions = {
            nop_2();
            set_egress_bd_properties_0();
            @defaultonly NoAction();
        }
        key = {
            meta.egress_metadata.bd: exact @name("egress_metadata.bd") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        egress_bd_map_0.apply();
    }
}

control process_rewrite(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_3() {
    }
    @name(".set_l2_rewrite") action set_l2_rewrite_0() {
        meta.egress_metadata.routed = 1w0;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.egress_metadata.outer_bd = meta.ingress_metadata.bd;
    }
    @name(".set_l2_rewrite_with_tunnel") action set_l2_rewrite_with_tunnel_0(bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.routed = 1w0;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.egress_metadata.outer_bd = meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".set_l3_rewrite") action set_l3_rewrite_0(bit<16> bd, bit<8> mtu_index, bit<9> smac_idx, bit<48> dmac) {
        meta.egress_metadata.routed = 1w1;
        meta.egress_metadata.smac_idx = smac_idx;
        meta.egress_metadata.mac_da = dmac;
        meta.egress_metadata.bd = bd;
        meta.egress_metadata.outer_bd = bd;
        meta.l3_metadata.mtu_index = mtu_index;
    }
    @name(".set_l3_rewrite_with_tunnel") action set_l3_rewrite_with_tunnel_0(bit<16> bd, bit<9> smac_idx, bit<48> dmac, bit<14> tunnel_index, bit<5> tunnel_type) {
        meta.egress_metadata.routed = 1w1;
        meta.egress_metadata.smac_idx = smac_idx;
        meta.egress_metadata.mac_da = dmac;
        meta.egress_metadata.bd = bd;
        meta.egress_metadata.outer_bd = bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_tunnel_type = tunnel_type;
    }
    @name(".set_mpls_swap_push_rewrite_l2") action set_mpls_swap_push_rewrite_l2_0(bit<20> label, bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        hdr.mpls[0].label = label;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w13;
    }
    @name(".set_mpls_push_rewrite_l2") action set_mpls_push_rewrite_l2_0(bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = meta.ingress_metadata.bd;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w13;
    }
    @name(".set_mpls_swap_push_rewrite_l3") action set_mpls_swap_push_rewrite_l3_0(bit<16> bd, bit<9> smac_idx, bit<48> dmac, bit<20> label, bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = bd;
        hdr.mpls[0].label = label;
        meta.egress_metadata.smac_idx = smac_idx;
        meta.egress_metadata.mac_da = dmac;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w14;
    }
    @name(".set_mpls_push_rewrite_l3") action set_mpls_push_rewrite_l3_0(bit<16> bd, bit<9> smac_idx, bit<48> dmac, bit<14> tunnel_index, bit<4> header_count) {
        meta.egress_metadata.routed = meta.l3_metadata.routed;
        meta.egress_metadata.bd = bd;
        meta.egress_metadata.smac_idx = smac_idx;
        meta.egress_metadata.mac_da = dmac;
        meta.tunnel_metadata.tunnel_index = tunnel_index;
        meta.tunnel_metadata.egress_header_count = header_count;
        meta.tunnel_metadata.egress_tunnel_type = 5w14;
    }
    @name(".rewrite") table rewrite_0 {
        actions = {
            nop_3();
            set_l2_rewrite_0();
            set_l2_rewrite_with_tunnel_0();
            set_l3_rewrite_0();
            set_l3_rewrite_with_tunnel_0();
            set_mpls_swap_push_rewrite_l2_0();
            set_mpls_push_rewrite_l2_0();
            set_mpls_swap_push_rewrite_l3_0();
            set_mpls_push_rewrite_l3_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("l3_metadata.nexthop_index") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        rewrite_0.apply();
    }
}

control process_int_insertion(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".int_set_header_0_bos") action int_set_header_0_bos_0() {
        hdr.int_switch_id_header.bos = 1w1;
    }
    @name(".int_set_header_1_bos") action int_set_header_1_bos_0() {
        hdr.int_ingress_port_id_header.bos = 1w1;
    }
    @name(".int_set_header_2_bos") action int_set_header_2_bos_0() {
        hdr.int_hop_latency_header.bos = 1w1;
    }
    @name(".int_set_header_3_bos") action int_set_header_3_bos_0() {
        hdr.int_q_occupancy_header.bos = 1w1;
    }
    @name(".int_set_header_4_bos") action int_set_header_4_bos_0() {
        hdr.int_ingress_tstamp_header.bos = 1w1;
    }
    @name(".int_set_header_5_bos") action int_set_header_5_bos_0() {
        hdr.int_egress_port_id_header.bos = 1w1;
    }
    @name(".int_set_header_6_bos") action int_set_header_6_bos_0() {
        hdr.int_q_congestion_header.bos = 1w1;
    }
    @name(".int_set_header_7_bos") action int_set_header_7_bos_0() {
        hdr.int_egress_port_tx_utilization_header.bos = 1w1;
    }
    @name(".nop") action nop_4() {
    }
    @name(".int_transit") action int_transit_0(bit<32> switch_id) {
        meta.int_metadata.insert_cnt = hdr.int_header.max_hop_cnt - hdr.int_header.total_hop_cnt;
        meta.int_metadata.switch_id = switch_id;
        meta.int_metadata.insert_byte_cnt = meta.int_metadata.instruction_cnt << 2;
        meta.int_metadata.gpe_int_hdr_len8 = (bit<8>)hdr.int_header.ins_cnt;
    }
    @name(".int_reset") action int_reset_0() {
        meta.int_metadata.switch_id = 32w0;
        meta.int_metadata.insert_byte_cnt = 16w0;
        meta.int_metadata.insert_cnt = 8w0;
        meta.int_metadata.gpe_int_hdr_len8 = 8w0;
        meta.int_metadata.gpe_int_hdr_len = 16w0;
        meta.int_metadata.instruction_cnt = 16w0;
    }
    @name(".int_set_header_0003_i0") action int_set_header_0003_i0_0() {
    }
    @name(".int_set_header_3") action int_set_header() {
        hdr.int_q_occupancy_header.setValid();
        hdr.int_q_occupancy_header.q_occupancy = (bit<31>)meta.intrinsic_metadata.enq_qdepth;
    }
    @name(".int_set_header_0003_i1") action int_set_header_0003_i1_0() {
        int_set_header();
    }
    @name(".int_set_header_2") action int_set_header_8() {
        hdr.int_hop_latency_header.setValid();
        hdr.int_hop_latency_header.hop_latency = (bit<31>)meta.intrinsic_metadata.deq_timedelta;
    }
    @name(".int_set_header_0003_i2") action int_set_header_0003_i2_0() {
        int_set_header_8();
    }
    @name(".int_set_header_0003_i3") action int_set_header_0003_i3_0() {
        int_set_header();
        int_set_header_8();
    }
    @name(".int_set_header_1") action int_set_header_9() {
        hdr.int_ingress_port_id_header.setValid();
        hdr.int_ingress_port_id_header.ingress_port_id = (bit<31>)meta.ingress_metadata.ifindex;
    }
    @name(".int_set_header_0003_i4") action int_set_header_0003_i4_0() {
        int_set_header_9();
    }
    @name(".int_set_header_0003_i5") action int_set_header_0003_i5_0() {
        int_set_header();
        int_set_header_9();
    }
    @name(".int_set_header_0003_i6") action int_set_header_0003_i6_0() {
        int_set_header_8();
        int_set_header_9();
    }
    @name(".int_set_header_0003_i7") action int_set_header_0003_i7_0() {
        int_set_header();
        int_set_header_8();
        int_set_header_9();
    }
    @name(".int_set_header_0") action int_set_header_10() {
        hdr.int_switch_id_header.setValid();
        hdr.int_switch_id_header.switch_id = (bit<31>)meta.int_metadata.switch_id;
    }
    @name(".int_set_header_0003_i8") action int_set_header_0003_i8_0() {
        int_set_header_10();
    }
    @name(".int_set_header_0003_i9") action int_set_header_0003_i9_0() {
        int_set_header();
        int_set_header_10();
    }
    @name(".int_set_header_0003_i10") action int_set_header_0003_i10_0() {
        int_set_header_8();
        int_set_header_10();
    }
    @name(".int_set_header_0003_i11") action int_set_header_0003_i11_0() {
        int_set_header();
        int_set_header_8();
        int_set_header_10();
    }
    @name(".int_set_header_0003_i12") action int_set_header_0003_i12_0() {
        int_set_header_9();
        int_set_header_10();
    }
    @name(".int_set_header_0003_i13") action int_set_header_0003_i13_0() {
        int_set_header();
        int_set_header_9();
        int_set_header_10();
    }
    @name(".int_set_header_0003_i14") action int_set_header_0003_i14_0() {
        int_set_header_8();
        int_set_header_9();
        int_set_header_10();
    }
    @name(".int_set_header_0003_i15") action int_set_header_0003_i15_0() {
        int_set_header();
        int_set_header_8();
        int_set_header_9();
        int_set_header_10();
    }
    @name(".int_set_header_0407_i0") action int_set_header_0407_i0_0() {
    }
    @name(".int_set_header_7") action int_set_header_11() {
        hdr.int_egress_port_tx_utilization_header.setValid();
        hdr.int_egress_port_tx_utilization_header.egress_port_tx_utilization = 31w0x7fffffff;
    }
    @name(".int_set_header_0407_i1") action int_set_header_0407_i1_0() {
        int_set_header_11();
    }
    @name(".int_set_header_6") action int_set_header_12() {
        hdr.int_q_congestion_header.setValid();
        hdr.int_q_congestion_header.q_congestion = 31w0x7fffffff;
    }
    @name(".int_set_header_0407_i2") action int_set_header_0407_i2_0() {
        int_set_header_12();
    }
    @name(".int_set_header_0407_i3") action int_set_header_0407_i3_0() {
        int_set_header_11();
        int_set_header_12();
    }
    @name(".int_set_header_5") action int_set_header_13() {
        hdr.int_egress_port_id_header.setValid();
        hdr.int_egress_port_id_header.egress_port_id = (bit<31>)standard_metadata.egress_port;
    }
    @name(".int_set_header_0407_i4") action int_set_header_0407_i4_0() {
        int_set_header_13();
    }
    @name(".int_set_header_0407_i5") action int_set_header_0407_i5_0() {
        int_set_header_11();
        int_set_header_13();
    }
    @name(".int_set_header_0407_i6") action int_set_header_0407_i6_0() {
        int_set_header_12();
        int_set_header_13();
    }
    @name(".int_set_header_0407_i7") action int_set_header_0407_i7_0() {
        int_set_header_11();
        int_set_header_12();
        int_set_header_13();
    }
    @name(".int_set_header_4") action int_set_header_14() {
        hdr.int_ingress_tstamp_header.setValid();
        hdr.int_ingress_tstamp_header.ingress_tstamp = (bit<31>)meta.i2e_metadata.ingress_tstamp;
    }
    @name(".int_set_header_0407_i8") action int_set_header_0407_i8_0() {
        int_set_header_14();
    }
    @name(".int_set_header_0407_i9") action int_set_header_0407_i9_0() {
        int_set_header_11();
        int_set_header_14();
    }
    @name(".int_set_header_0407_i10") action int_set_header_0407_i10_0() {
        int_set_header_12();
        int_set_header_14();
    }
    @name(".int_set_header_0407_i11") action int_set_header_0407_i11_0() {
        int_set_header_11();
        int_set_header_12();
        int_set_header_14();
    }
    @name(".int_set_header_0407_i12") action int_set_header_0407_i12_0() {
        int_set_header_13();
        int_set_header_14();
    }
    @name(".int_set_header_0407_i13") action int_set_header_0407_i13_0() {
        int_set_header_11();
        int_set_header_13();
        int_set_header_14();
    }
    @name(".int_set_header_0407_i14") action int_set_header_0407_i14_0() {
        int_set_header_12();
        int_set_header_13();
        int_set_header_14();
    }
    @name(".int_set_header_0407_i15") action int_set_header_0407_i15_0() {
        int_set_header_11();
        int_set_header_12();
        int_set_header_13();
        int_set_header_14();
    }
    @name(".int_set_e_bit") action int_set_e_bit_0() {
        hdr.int_header.e = 1w1;
    }
    @name(".int_update_total_hop_cnt") action int_update_total_hop_cnt_0() {
        hdr.int_header.total_hop_cnt = hdr.int_header.total_hop_cnt + 8w1;
    }
    @name(".int_bos") table int_bos_0 {
        actions = {
            int_set_header_0_bos_0();
            int_set_header_1_bos_0();
            int_set_header_2_bos_0();
            int_set_header_3_bos_0();
            int_set_header_4_bos_0();
            int_set_header_5_bos_0();
            int_set_header_6_bos_0();
            int_set_header_7_bos_0();
            nop_4();
            @defaultonly NoAction();
        }
        key = {
            hdr.int_header.total_hop_cnt        : ternary @name("int_header.total_hop_cnt") ;
            hdr.int_header.instruction_mask_0003: ternary @name("int_header.instruction_mask_0003") ;
            hdr.int_header.instruction_mask_0407: ternary @name("int_header.instruction_mask_0407") ;
            hdr.int_header.instruction_mask_0811: ternary @name("int_header.instruction_mask_0811") ;
            hdr.int_header.instruction_mask_1215: ternary @name("int_header.instruction_mask_1215") ;
        }
        size = 16;
        default_action = NoAction();
    }
    @name(".int_insert") table int_insert_0 {
        actions = {
            int_transit_0();
            int_reset_0();
            @defaultonly NoAction();
        }
        key = {
            meta.int_metadata_i2e.source: ternary @name("int_metadata_i2e.source") ;
            meta.int_metadata_i2e.sink  : ternary @name("int_metadata_i2e.sink") ;
            hdr.int_header.isValid()    : exact @name("int_header.$valid$") ;
        }
        size = 2;
        default_action = NoAction();
    }
    @name(".int_inst_0003") table int_inst {
        actions = {
            int_set_header_0003_i0_0();
            int_set_header_0003_i1_0();
            int_set_header_0003_i2_0();
            int_set_header_0003_i3_0();
            int_set_header_0003_i4_0();
            int_set_header_0003_i5_0();
            int_set_header_0003_i6_0();
            int_set_header_0003_i7_0();
            int_set_header_0003_i8_0();
            int_set_header_0003_i9_0();
            int_set_header_0003_i10_0();
            int_set_header_0003_i11_0();
            int_set_header_0003_i12_0();
            int_set_header_0003_i13_0();
            int_set_header_0003_i14_0();
            int_set_header_0003_i15_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.int_header.instruction_mask_0003: exact @name("int_header.instruction_mask_0003") ;
        }
        size = 16;
        default_action = NoAction();
    }
    @name(".int_inst_0407") table int_inst_0 {
        actions = {
            int_set_header_0407_i0_0();
            int_set_header_0407_i1_0();
            int_set_header_0407_i2_0();
            int_set_header_0407_i3_0();
            int_set_header_0407_i4_0();
            int_set_header_0407_i5_0();
            int_set_header_0407_i6_0();
            int_set_header_0407_i7_0();
            int_set_header_0407_i8_0();
            int_set_header_0407_i9_0();
            int_set_header_0407_i10_0();
            int_set_header_0407_i11_0();
            int_set_header_0407_i12_0();
            int_set_header_0407_i13_0();
            int_set_header_0407_i14_0();
            int_set_header_0407_i15_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.int_header.instruction_mask_0407: exact @name("int_header.instruction_mask_0407") ;
        }
        size = 16;
        default_action = NoAction();
    }
    @name(".int_inst_0811") table int_inst_1 {
        actions = {
            nop_4();
            @defaultonly NoAction();
        }
        key = {
            hdr.int_header.instruction_mask_0811: exact @name("int_header.instruction_mask_0811") ;
        }
        size = 16;
        default_action = NoAction();
    }
    @name(".int_inst_1215") table int_inst_2 {
        actions = {
            nop_4();
            @defaultonly NoAction();
        }
        key = {
            hdr.int_header.instruction_mask_1215: exact @name("int_header.instruction_mask_1215") ;
        }
        size = 16;
        default_action = NoAction();
    }
    @name(".int_meta_header_update") table int_meta_header_update_0 {
        actions = {
            int_set_e_bit_0();
            int_update_total_hop_cnt_0();
            @defaultonly NoAction();
        }
        key = {
            meta.int_metadata.insert_cnt: ternary @name("int_metadata.insert_cnt") ;
        }
        size = 1;
        default_action = NoAction();
    }
    apply {
        switch (int_insert_0.apply().action_run) {
            int_transit_0: {
                if (meta.int_metadata.insert_cnt != 8w0) {
                    int_inst.apply();
                    int_inst_0.apply();
                    int_inst_1.apply();
                    int_inst_2.apply();
                    int_bos_0.apply();
                }
                int_meta_header_update_0.apply();
            }
        }

    }
}

control process_mac_rewrite(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_5() {
    }
    @name(".rewrite_ipv4_unicast_mac") action rewrite_ipv4_unicast_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".rewrite_ipv4_multicast_mac") action rewrite_ipv4_multicast_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr[47:23] = 25w0x0;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".rewrite_ipv6_unicast_mac") action rewrite_ipv6_unicast_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit + 8w255;
    }
    @name(".rewrite_ipv6_multicast_mac") action rewrite_ipv6_multicast_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr[47:32] = 16w0x0;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit + 8w255;
    }
    @name(".rewrite_mpls_mac") action rewrite_mpls_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.mpls[0].ttl = hdr.mpls[0].ttl + 8w255;
    }
    @name(".mac_rewrite") table mac_rewrite_0 {
        actions = {
            nop_5();
            rewrite_ipv4_unicast_mac_0();
            rewrite_ipv4_multicast_mac_0();
            rewrite_ipv6_unicast_mac_0();
            rewrite_ipv6_multicast_mac_0();
            rewrite_mpls_mac_0();
            @defaultonly NoAction();
        }
        key = {
            meta.egress_metadata.smac_idx: exact @name("egress_metadata.smac_idx") ;
            hdr.ipv4.isValid()           : exact @name("ipv4.$valid$") ;
            hdr.ipv6.isValid()           : exact @name("ipv6.$valid$") ;
            hdr.mpls[0].isValid()        : exact @name("mpls[0].$valid$") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        if (meta.egress_metadata.routed == 1w1) 
            mac_rewrite_0.apply();
    }
}

control process_tunnel_encap(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_6() {
    }
    @name(".set_egress_tunnel_vni") action set_egress_tunnel_vni_0(bit<24> vnid) {
        meta.tunnel_metadata.vnid = vnid;
    }
    @name(".rewrite_tunnel_dmac") action rewrite_tunnel_dmac_0(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name(".rewrite_tunnel_ipv4_dst") action rewrite_tunnel_ipv4_dst_0(bit<32> ip) {
        hdr.ipv4.dstAddr = ip;
    }
    @name(".rewrite_tunnel_ipv6_dst") action rewrite_tunnel_ipv6_dst_0(bit<128> ip) {
        hdr.ipv6.dstAddr = ip;
    }
    @name(".inner_ipv4_udp_rewrite") action inner_ipv4_udp_rewrite_0() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_udp = hdr.udp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.udp.setInvalid();
        hdr.ipv4.setInvalid();
    }
    @name(".inner_ipv4_tcp_rewrite") action inner_ipv4_tcp_rewrite_0() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_tcp = hdr.tcp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.tcp.setInvalid();
        hdr.ipv4.setInvalid();
    }
    @name(".inner_ipv4_icmp_rewrite") action inner_ipv4_icmp_rewrite_0() {
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.inner_icmp = hdr.icmp;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.icmp.setInvalid();
        hdr.ipv4.setInvalid();
    }
    @name(".inner_ipv4_unknown_rewrite") action inner_ipv4_unknown_rewrite_0() {
        hdr.inner_ipv4 = hdr.ipv4;
        meta.egress_metadata.payload_length = hdr.ipv4.totalLen;
        hdr.ipv4.setInvalid();
    }
    @name(".inner_ipv6_udp_rewrite") action inner_ipv6_udp_rewrite_0() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_udp = hdr.udp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.ipv6.setInvalid();
    }
    @name(".inner_ipv6_tcp_rewrite") action inner_ipv6_tcp_rewrite_0() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_tcp = hdr.tcp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.tcp.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".inner_ipv6_icmp_rewrite") action inner_ipv6_icmp_rewrite_0() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.inner_icmp = hdr.icmp;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.icmp.setInvalid();
        hdr.ipv6.setInvalid();
    }
    @name(".inner_ipv6_unknown_rewrite") action inner_ipv6_unknown_rewrite_0() {
        hdr.inner_ipv6 = hdr.ipv6;
        meta.egress_metadata.payload_length = hdr.ipv6.payloadLen + 16w40;
        hdr.ipv6.setInvalid();
    }
    @name(".inner_non_ip_rewrite") action inner_non_ip_rewrite_0() {
        meta.egress_metadata.payload_length = (bit<16>)standard_metadata.packet_length + 16w65522;
    }
    @name(".f_insert_vxlan_header") action f_insert_vxlan_header_0() {
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
    @name(".f_insert_ipv4_header") action f_insert_ipv4_header_0(bit<8> proto_0) {
        hdr.ipv4.setValid();
        hdr.ipv4.protocol = proto_0;
        hdr.ipv4.ttl = 8w64;
        hdr.ipv4.version = 4w0x4;
        hdr.ipv4.ihl = 4w0x5;
        hdr.ipv4.identification = 16w0;
    }
    @name(".ipv4_vxlan_rewrite") action ipv4_vxlan_rewrite_0() {
        f_insert_vxlan_header_0();
        f_insert_ipv4_header_0(8w17);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w50;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".f_insert_ipv6_header") action f_insert_ipv6_header_0(bit<8> proto_1) {
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w0x6;
        hdr.ipv6.nextHdr = proto_1;
        hdr.ipv6.hopLimit = 8w64;
        hdr.ipv6.trafficClass = 8w0;
        hdr.ipv6.flowLabel = 20w0;
    }
    @name(".ipv6_vxlan_rewrite") action ipv6_vxlan_rewrite_0() {
        f_insert_vxlan_header_0();
        f_insert_ipv6_header_0(8w17);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w30;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".f_insert_genv_header") action f_insert_genv_header_0() {
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
    @name(".ipv4_genv_rewrite") action ipv4_genv_rewrite_0() {
        f_insert_genv_header_0();
        f_insert_ipv4_header_0(8w17);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w50;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv6_genv_rewrite") action ipv6_genv_rewrite_0() {
        f_insert_genv_header_0();
        f_insert_ipv6_header_0(8w17);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w30;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".f_insert_nvgre_header") action f_insert_nvgre_header_0() {
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
    @name(".ipv4_nvgre_rewrite") action ipv4_nvgre_rewrite_0() {
        f_insert_nvgre_header_0();
        f_insert_ipv4_header_0(8w47);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w42;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv6_nvgre_rewrite") action ipv6_nvgre_rewrite_0() {
        f_insert_nvgre_header_0();
        f_insert_ipv6_header_0(8w47);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w22;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".f_insert_gre_header") action f_insert_gre_header_0() {
        hdr.gre.setValid();
    }
    @name(".ipv4_gre_rewrite") action ipv4_gre_rewrite_0() {
        f_insert_gre_header_0();
        hdr.gre.proto = hdr.ethernet.etherType;
        f_insert_ipv4_header_0(8w47);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w38;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv6_gre_rewrite") action ipv6_gre_rewrite_0() {
        f_insert_gre_header_0();
        hdr.gre.proto = 16w0x800;
        f_insert_ipv6_header_0(8w47);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w18;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv4_ipv4_rewrite") action ipv4_ipv4_rewrite_0() {
        f_insert_ipv4_header_0(8w4);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w20;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv4_ipv6_rewrite") action ipv4_ipv6_rewrite_0() {
        f_insert_ipv4_header_0(8w41);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w40;
        hdr.ethernet.etherType = 16w0x800;
    }
    @name(".ipv6_ipv4_rewrite") action ipv6_ipv4_rewrite_0() {
        f_insert_ipv6_header_0(8w4);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w20;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".ipv6_ipv6_rewrite") action ipv6_ipv6_rewrite_0() {
        f_insert_ipv6_header_0(8w41);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w40;
        hdr.ethernet.etherType = 16w0x86dd;
    }
    @name(".f_insert_erspan_t3_header") action f_insert_erspan_t3_header_0() {
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
    @name(".ipv4_erspan_t3_rewrite") action ipv4_erspan_t3_rewrite_0() {
        f_insert_erspan_t3_header_0();
        f_insert_ipv4_header_0(8w47);
        hdr.ipv4.totalLen = meta.egress_metadata.payload_length + 16w50;
    }
    @name(".ipv6_erspan_t3_rewrite") action ipv6_erspan_t3_rewrite_0() {
        f_insert_erspan_t3_header_0();
        f_insert_ipv6_header_0(8w47);
        hdr.ipv6.payloadLen = meta.egress_metadata.payload_length + 16w26;
    }
    @name(".mpls_ethernet_push1_rewrite") action mpls_ethernet_push1_rewrite_0() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.mpls.push_front(1);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push1_rewrite") action mpls_ip_push1_rewrite_0() {
        hdr.mpls.push_front(1);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ethernet_push2_rewrite") action mpls_ethernet_push2_rewrite_0() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.mpls.push_front(2);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push2_rewrite") action mpls_ip_push2_rewrite_0() {
        hdr.mpls.push_front(2);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ethernet_push3_rewrite") action mpls_ethernet_push3_rewrite_0() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.mpls.push_front(3);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".mpls_ip_push3_rewrite") action mpls_ip_push3_rewrite_0() {
        hdr.mpls.push_front(3);
        hdr.ethernet.etherType = 16w0x8847;
    }
    @name(".fabric_rewrite") action fabric_rewrite_0(bit<14> tunnel_index) {
        meta.tunnel_metadata.tunnel_index = tunnel_index;
    }
    @name(".set_tunnel_rewrite_details") action set_tunnel_rewrite_details_0(bit<16> outer_bd, bit<8> mtu_index, bit<9> smac_idx, bit<14> dmac_idx, bit<9> sip_index, bit<14> dip_index) {
        meta.egress_metadata.outer_bd = outer_bd;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx;
        meta.tunnel_metadata.tunnel_src_index = sip_index;
        meta.tunnel_metadata.tunnel_dst_index = dip_index;
        meta.l3_metadata.mtu_index = mtu_index;
    }
    @name(".set_mpls_rewrite_push1") action set_mpls_rewrite_push1_0(bit<20> label1, bit<3> exp1, bit<8> ttl1, bit<9> smac_idx, bit<14> dmac_idx) {
        hdr.mpls[0].label = label1;
        hdr.mpls[0].exp = exp1;
        hdr.mpls[0].bos = 1w0x1;
        hdr.mpls[0].ttl = ttl1;
        meta.tunnel_metadata.tunnel_smac_index = smac_idx;
        meta.tunnel_metadata.tunnel_dmac_index = dmac_idx;
    }
    @name(".set_mpls_rewrite_push2") action set_mpls_rewrite_push2_0(bit<20> label1, bit<3> exp1, bit<8> ttl1, bit<20> label2, bit<3> exp2, bit<8> ttl2, bit<9> smac_idx, bit<14> dmac_idx) {
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
    @name(".set_mpls_rewrite_push3") action set_mpls_rewrite_push3_0(bit<20> label1, bit<3> exp1, bit<8> ttl1, bit<20> label2, bit<3> exp2, bit<8> ttl2, bit<20> label3, bit<3> exp3, bit<8> ttl3, bit<9> smac_idx, bit<14> dmac_idx) {
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
    @name(".cpu_rx_rewrite") action cpu_rx_rewrite_0() {
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
    @name(".fabric_unicast_rewrite") action fabric_unicast_rewrite_0() {
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
    @name(".fabric_multicast_rewrite") action fabric_multicast_rewrite_0(bit<16> fabric_mgid) {
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
    @name(".rewrite_tunnel_smac") action rewrite_tunnel_smac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name(".rewrite_tunnel_ipv4_src") action rewrite_tunnel_ipv4_src_0(bit<32> ip) {
        hdr.ipv4.srcAddr = ip;
    }
    @name(".rewrite_tunnel_ipv6_src") action rewrite_tunnel_ipv6_src_0(bit<128> ip) {
        hdr.ipv6.srcAddr = ip;
    }
    @name(".egress_vni") table egress_vni_0 {
        actions = {
            nop_6();
            set_egress_tunnel_vni_0();
            @defaultonly NoAction();
        }
        key = {
            meta.egress_metadata.bd                : exact @name("egress_metadata.bd") ;
            meta.tunnel_metadata.egress_tunnel_type: exact @name("tunnel_metadata.egress_tunnel_type") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunnel_dmac_rewrite") table tunnel_dmac_rewrite_0 {
        actions = {
            nop_6();
            rewrite_tunnel_dmac_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.tunnel_dmac_index: exact @name("tunnel_metadata.tunnel_dmac_index") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunnel_dst_rewrite") table tunnel_dst_rewrite_0 {
        actions = {
            nop_6();
            rewrite_tunnel_ipv4_dst_0();
            rewrite_tunnel_ipv6_dst_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.tunnel_dst_index: exact @name("tunnel_metadata.tunnel_dst_index") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunnel_encap_process_inner") table tunnel_encap_process_inner_0 {
        actions = {
            inner_ipv4_udp_rewrite_0();
            inner_ipv4_tcp_rewrite_0();
            inner_ipv4_icmp_rewrite_0();
            inner_ipv4_unknown_rewrite_0();
            inner_ipv6_udp_rewrite_0();
            inner_ipv6_tcp_rewrite_0();
            inner_ipv6_icmp_rewrite_0();
            inner_ipv6_unknown_rewrite_0();
            inner_non_ip_rewrite_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.ipv4.isValid(): exact @name("ipv4.$valid$") ;
            hdr.ipv6.isValid(): exact @name("ipv6.$valid$") ;
            hdr.tcp.isValid() : exact @name("tcp.$valid$") ;
            hdr.udp.isValid() : exact @name("udp.$valid$") ;
            hdr.icmp.isValid(): exact @name("icmp.$valid$") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunnel_encap_process_outer") table tunnel_encap_process_outer_0 {
        actions = {
            nop_6();
            ipv4_vxlan_rewrite_0();
            ipv6_vxlan_rewrite_0();
            ipv4_genv_rewrite_0();
            ipv6_genv_rewrite_0();
            ipv4_nvgre_rewrite_0();
            ipv6_nvgre_rewrite_0();
            ipv4_gre_rewrite_0();
            ipv6_gre_rewrite_0();
            ipv4_ipv4_rewrite_0();
            ipv4_ipv6_rewrite_0();
            ipv6_ipv4_rewrite_0();
            ipv6_ipv6_rewrite_0();
            ipv4_erspan_t3_rewrite_0();
            ipv6_erspan_t3_rewrite_0();
            mpls_ethernet_push1_rewrite_0();
            mpls_ip_push1_rewrite_0();
            mpls_ethernet_push2_rewrite_0();
            mpls_ip_push2_rewrite_0();
            mpls_ethernet_push3_rewrite_0();
            mpls_ip_push3_rewrite_0();
            fabric_rewrite_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.egress_tunnel_type : exact @name("tunnel_metadata.egress_tunnel_type") ;
            meta.tunnel_metadata.egress_header_count: exact @name("tunnel_metadata.egress_header_count") ;
            meta.multicast_metadata.replica         : exact @name("multicast_metadata.replica") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunnel_rewrite") table tunnel_rewrite_0 {
        actions = {
            nop_6();
            set_tunnel_rewrite_details_0();
            set_mpls_rewrite_push1_0();
            set_mpls_rewrite_push2_0();
            set_mpls_rewrite_push3_0();
            cpu_rx_rewrite_0();
            fabric_unicast_rewrite_0();
            fabric_multicast_rewrite_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.tunnel_index: exact @name("tunnel_metadata.tunnel_index") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunnel_smac_rewrite") table tunnel_smac_rewrite_0 {
        actions = {
            nop_6();
            rewrite_tunnel_smac_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.tunnel_smac_index: exact @name("tunnel_metadata.tunnel_smac_index") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunnel_src_rewrite") table tunnel_src_rewrite_0 {
        actions = {
            nop_6();
            rewrite_tunnel_ipv4_src_0();
            rewrite_tunnel_ipv6_src_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.tunnel_src_index: exact @name("tunnel_metadata.tunnel_src_index") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        if (meta.fabric_metadata.fabric_header_present == 1w0 && meta.tunnel_metadata.egress_tunnel_type != 5w0) {
            egress_vni_0.apply();
            if (meta.tunnel_metadata.egress_tunnel_type != 5w15 && meta.tunnel_metadata.egress_tunnel_type != 5w16) 
                tunnel_encap_process_inner_0.apply();
            tunnel_encap_process_outer_0.apply();
            tunnel_rewrite_0.apply();
            tunnel_src_rewrite_0.apply();
            tunnel_dst_rewrite_0.apply();
            tunnel_smac_rewrite_0.apply();
            tunnel_dmac_rewrite_0.apply();
        }
    }
}

control process_int_outer_encap(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".int_update_vxlan_gpe_ipv4") action int_update_vxlan_gpe_ipv4_0() {
        hdr.ipv4.totalLen = hdr.ipv4.totalLen + meta.int_metadata.insert_byte_cnt;
        hdr.udp.length_ = hdr.udp.length_ + meta.int_metadata.insert_byte_cnt;
        hdr.vxlan_gpe_int_header.len = hdr.vxlan_gpe_int_header.len + meta.int_metadata.gpe_int_hdr_len8;
    }
    @name(".nop") action nop_7() {
    }
    @name(".int_outer_encap") table int_outer_encap_0 {
        actions = {
            int_update_vxlan_gpe_ipv4_0();
            nop_7();
            @defaultonly NoAction();
        }
        key = {
            hdr.ipv4.isValid()                     : exact @name("ipv4.$valid$") ;
            hdr.vxlan_gpe.isValid()                : exact @name("vxlan_gpe.$valid$") ;
            meta.int_metadata_i2e.source           : exact @name("int_metadata_i2e.source") ;
            meta.tunnel_metadata.egress_tunnel_type: ternary @name("tunnel_metadata.egress_tunnel_type") ;
        }
        size = 8;
        default_action = NoAction();
    }
    apply {
        if (meta.int_metadata.insert_cnt != 8w0) 
            int_outer_encap_0.apply();
    }
}

control process_vlan_xlate(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_egress_packet_vlan_untagged") action set_egress_packet_vlan_untagged_0() {
    }
    @name(".set_egress_packet_vlan_tagged") action set_egress_packet_vlan_tagged_0(bit<12> vlan_id) {
        hdr.vlan_tag_[0].setValid();
        hdr.vlan_tag_[0].etherType = hdr.ethernet.etherType;
        hdr.vlan_tag_[0].vid = vlan_id;
        hdr.ethernet.etherType = 16w0x8100;
    }
    @name(".set_egress_packet_vlan_double_tagged") action set_egress_packet_vlan_double_tagged_0(bit<12> s_tag, bit<12> c_tag) {
        hdr.vlan_tag_[1].setValid();
        hdr.vlan_tag_[0].setValid();
        hdr.vlan_tag_[1].etherType = hdr.ethernet.etherType;
        hdr.vlan_tag_[1].vid = c_tag;
        hdr.vlan_tag_[0].etherType = 16w0x8100;
        hdr.vlan_tag_[0].vid = s_tag;
        hdr.ethernet.etherType = 16w0x9100;
    }
    @name(".egress_vlan_xlate") table egress_vlan_xlate_0 {
        actions = {
            set_egress_packet_vlan_untagged_0();
            set_egress_packet_vlan_tagged_0();
            set_egress_packet_vlan_double_tagged_0();
            @defaultonly NoAction();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
            meta.egress_metadata.bd      : exact @name("egress_metadata.bd") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        egress_vlan_xlate_0.apply();
    }
}

control process_egress_filter(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_egress_filter_drop") action set_egress_filter_drop_0() {
        mark_to_drop();
    }
    @name(".set_egress_ifindex") action set_egress_ifindex_0(bit<16> egress_ifindex) {
        meta.egress_filter_metadata.ifindex = meta.ingress_metadata.ifindex ^ egress_ifindex;
        meta.egress_filter_metadata.bd = meta.ingress_metadata.outer_bd ^ meta.egress_metadata.outer_bd;
        meta.egress_filter_metadata.inner_bd = meta.ingress_metadata.bd ^ meta.egress_metadata.bd;
    }
    @name(".egress_filter") table egress_filter_0 {
        actions = {
            set_egress_filter_drop_0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name(".egress_lag") table egress_lag_0 {
        actions = {
            set_egress_ifindex_0();
            @defaultonly NoAction();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
        }
        default_action = NoAction();
    }
    apply {
        egress_lag_0.apply();
        if (meta.multicast_metadata.inner_replica == 1w1) 
            if (meta.tunnel_metadata.egress_tunnel_type != 5w0 && meta.egress_filter_metadata.inner_bd == 16w0 || meta.egress_filter_metadata.ifindex == 16w0 && meta.egress_filter_metadata.bd == 16w0) 
                egress_filter_0.apply();
    }
}

control process_egress_acl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_8() {
    }
    @name(".egress_mirror") action egress_mirror_0(bit<32> session_id) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        clone3<tuple<bit<32>, bit<16>>>(CloneType.E2E, session_id, { meta.i2e_metadata.ingress_tstamp, meta.i2e_metadata.mirror_session_id });
    }
    @name(".egress_mirror_drop") action egress_mirror_drop_0(bit<32> session_id) {
        egress_mirror_0(session_id);
        mark_to_drop();
    }
    @name(".egress_copy_to_cpu") action egress_copy_to_cpu_0(bit<16> reason_code_0) {
        meta.fabric_metadata.reason_code = reason_code_0;
        clone3<tuple<bit<16>, bit<16>, bit<16>, bit<9>>>(CloneType.E2E, 32w250, { meta.ingress_metadata.bd, meta.ingress_metadata.ifindex, meta.fabric_metadata.reason_code, meta.ingress_metadata.ingress_port });
    }
    @name(".egress_redirect_to_cpu") action egress_redirect_to_cpu_0(bit<16> reason_code) {
        egress_copy_to_cpu_0(reason_code);
        mark_to_drop();
    }
    @name(".egress_acl") table egress_acl_0 {
        actions = {
            nop_8();
            egress_mirror_0();
            egress_mirror_drop_0();
            egress_redirect_to_cpu_0();
            @defaultonly NoAction();
        }
        key = {
            standard_metadata.egress_port          : ternary @name("standard_metadata.egress_port") ;
            meta.intrinsic_metadata.deflection_flag: ternary @name("intrinsic_metadata.deflection_flag") ;
            meta.l3_metadata.l3_mtu_check          : ternary @name("l3_metadata.l3_mtu_check") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        egress_acl_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
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
    @name(".nop") action nop_9() {
    }
    @name(".set_mirror_nhop") action set_mirror_nhop_0(bit<16> nhop_idx) {
        meta.l3_metadata.nexthop_index = nhop_idx;
    }
    @name(".set_mirror_bd") action set_mirror_bd_0(bit<16> bd) {
        meta.egress_metadata.bd = bd;
    }
    @name(".egress_port_mapping") table egress_port_mapping_0 {
        actions = {
            egress_port_type_normal_0();
            egress_port_type_fabric_0();
            egress_port_type_cpu_0();
            @defaultonly NoAction();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
        }
        size = 288;
        default_action = NoAction();
    }
    @name(".mirror") table mirror_0 {
        actions = {
            nop_9();
            set_mirror_nhop_0();
            set_mirror_bd_0();
            @defaultonly NoAction();
        }
        key = {
            meta.i2e_metadata.mirror_session_id: exact @name("i2e_metadata.mirror_session_id") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".process_replication") process_replication() process_replication_1;
    @name(".process_vlan_decap") process_vlan_decap() process_vlan_decap_1;
    @name(".process_tunnel_decap") process_tunnel_decap() process_tunnel_decap_1;
    @name(".process_egress_bd") process_egress_bd() process_egress_bd_1;
    @name(".process_rewrite") process_rewrite() process_rewrite_1;
    @name(".process_int_insertion") process_int_insertion() process_int_insertion_1;
    @name(".process_mac_rewrite") process_mac_rewrite() process_mac_rewrite_1;
    @name(".process_tunnel_encap") process_tunnel_encap() process_tunnel_encap_1;
    @name(".process_int_outer_encap") process_int_outer_encap() process_int_outer_encap_1;
    @name(".process_vlan_xlate") process_vlan_xlate() process_vlan_xlate_1;
    @name(".process_egress_filter") process_egress_filter() process_egress_filter_1;
    @name(".process_egress_acl") process_egress_acl() process_egress_acl_1;
    apply {
        if (meta.intrinsic_metadata.deflection_flag == 1w0 && meta.egress_metadata.bypass == 1w0) {
            if (standard_metadata.instance_type != 32w0 && standard_metadata.instance_type != 32w5) 
                mirror_0.apply();
            else 
                process_replication_1.apply(hdr, meta, standard_metadata);
            switch (egress_port_mapping_0.apply().action_run) {
                egress_port_type_normal_0: {
                    if (standard_metadata.instance_type == 32w0 || standard_metadata.instance_type == 32w5) 
                        process_vlan_decap_1.apply(hdr, meta, standard_metadata);
                    process_tunnel_decap_1.apply(hdr, meta, standard_metadata);
                    process_egress_bd_1.apply(hdr, meta, standard_metadata);
                    process_rewrite_1.apply(hdr, meta, standard_metadata);
                    process_int_insertion_1.apply(hdr, meta, standard_metadata);
                    process_mac_rewrite_1.apply(hdr, meta, standard_metadata);
                }
            }

            process_tunnel_encap_1.apply(hdr, meta, standard_metadata);
            process_int_outer_encap_1.apply(hdr, meta, standard_metadata);
            if (meta.egress_metadata.port_type == 2w0) 
                process_vlan_xlate_1.apply(hdr, meta, standard_metadata);
            process_egress_filter_1.apply(hdr, meta, standard_metadata);
        }
        process_egress_acl_1.apply(hdr, meta, standard_metadata);
    }
}

control process_ingress_port_mapping(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_ifindex") action set_ifindex_0(bit<16> ifindex, bit<15> if_label, bit<2> port_type) {
        meta.ingress_metadata.ifindex = ifindex;
        meta.acl_metadata.if_label = if_label;
        meta.ingress_metadata.port_type = port_type;
    }
    @name(".ingress_port_mapping") table ingress_port_mapping_0 {
        actions = {
            set_ifindex_0();
            @defaultonly NoAction();
        }
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port") ;
        }
        size = 288;
        default_action = NoAction();
    }
    apply {
        ingress_port_mapping_0.apply();
    }
}

control validate_outer_ipv4_header(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_valid_outer_ipv4_packet") action set_valid_outer_ipv4_packet_0() {
        meta.l3_metadata.lkp_ip_type = 2w1;
        meta.l3_metadata.lkp_ip_tc = hdr.ipv4.diffserv;
        meta.l3_metadata.lkp_ip_version = 4w4;
    }
    @name(".set_malformed_outer_ipv4_packet") action set_malformed_outer_ipv4_packet_0(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
    }
    @name(".validate_outer_ipv4_packet") table validate_outer_ipv4_packet_0 {
        actions = {
            set_valid_outer_ipv4_packet_0();
            set_malformed_outer_ipv4_packet_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.ipv4.version                     : ternary @name("ipv4.version") ;
            meta.l3_metadata.lkp_ip_ttl          : ternary @name("l3_metadata.lkp_ip_ttl") ;
            meta.ipv4_metadata.lkp_ipv4_sa[31:24]: ternary @name("ipv4_metadata.lkp_ipv4_sa[31:24]") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        validate_outer_ipv4_packet_0.apply();
    }
}

control validate_outer_ipv6_header(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_valid_outer_ipv6_packet") action set_valid_outer_ipv6_packet_0() {
        meta.l3_metadata.lkp_ip_type = 2w2;
        meta.l3_metadata.lkp_ip_tc = hdr.ipv6.trafficClass;
        meta.l3_metadata.lkp_ip_version = 4w6;
    }
    @name(".set_malformed_outer_ipv6_packet") action set_malformed_outer_ipv6_packet_0(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
    }
    @name(".validate_outer_ipv6_packet") table validate_outer_ipv6_packet_0 {
        actions = {
            set_valid_outer_ipv6_packet_0();
            set_malformed_outer_ipv6_packet_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.ipv6.version                       : ternary @name("ipv6.version") ;
            meta.l3_metadata.lkp_ip_ttl            : ternary @name("l3_metadata.lkp_ip_ttl") ;
            meta.ipv6_metadata.lkp_ipv6_sa[127:112]: ternary @name("ipv6_metadata.lkp_ipv6_sa[127:112]") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        validate_outer_ipv6_packet_0.apply();
    }
}

control validate_mpls_header(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_valid_mpls_label1") action set_valid_mpls_label1_0() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[0].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[0].exp;
    }
    @name(".set_valid_mpls_label2") action set_valid_mpls_label2_0() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[1].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[1].exp;
    }
    @name(".set_valid_mpls_label3") action set_valid_mpls_label3_0() {
        meta.tunnel_metadata.mpls_label = hdr.mpls[2].label;
        meta.tunnel_metadata.mpls_exp = hdr.mpls[2].exp;
    }
    @name(".validate_mpls_packet") table validate_mpls_packet_0 {
        actions = {
            set_valid_mpls_label1_0();
            set_valid_mpls_label2_0();
            set_valid_mpls_label3_0();
            @defaultonly NoAction();
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
        default_action = NoAction();
    }
    apply {
        validate_mpls_packet_0.apply();
    }
}

control process_validate_outer_header(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".malformed_outer_ethernet_packet") action malformed_outer_ethernet_packet_0(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_untagged") action set_valid_outer_unicast_packet_untagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_single_tagged") action set_valid_outer_unicast_packet_single_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_double_tagged") action set_valid_outer_unicast_packet_double_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_qinq_tagged") action set_valid_outer_unicast_packet_qinq_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_untagged") action set_valid_outer_multicast_packet_untagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_single_tagged") action set_valid_outer_multicast_packet_single_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_double_tagged") action set_valid_outer_multicast_packet_double_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_qinq_tagged") action set_valid_outer_multicast_packet_qinq_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_untagged") action set_valid_outer_broadcast_packet_untagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_single_tagged") action set_valid_outer_broadcast_packet_single_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_double_tagged") action set_valid_outer_broadcast_packet_double_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_qinq_tagged") action set_valid_outer_broadcast_packet_qinq_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".validate_outer_ethernet") table validate_outer_ethernet_0 {
        actions = {
            malformed_outer_ethernet_packet_0();
            set_valid_outer_unicast_packet_untagged_0();
            set_valid_outer_unicast_packet_single_tagged_0();
            set_valid_outer_unicast_packet_double_tagged_0();
            set_valid_outer_unicast_packet_qinq_tagged_0();
            set_valid_outer_multicast_packet_untagged_0();
            set_valid_outer_multicast_packet_single_tagged_0();
            set_valid_outer_multicast_packet_double_tagged_0();
            set_valid_outer_multicast_packet_qinq_tagged_0();
            set_valid_outer_broadcast_packet_untagged_0();
            set_valid_outer_broadcast_packet_single_tagged_0();
            set_valid_outer_broadcast_packet_double_tagged_0();
            set_valid_outer_broadcast_packet_qinq_tagged_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l2_metadata.lkp_mac_sa: ternary @name("l2_metadata.lkp_mac_sa") ;
            meta.l2_metadata.lkp_mac_da: ternary @name("l2_metadata.lkp_mac_da") ;
            hdr.vlan_tag_[0].isValid() : exact @name("vlan_tag_[0].$valid$") ;
            hdr.vlan_tag_[1].isValid() : exact @name("vlan_tag_[1].$valid$") ;
        }
        size = 512;
        default_action = NoAction();
    }
    @name(".validate_outer_ipv4_header") validate_outer_ipv4_header() validate_outer_ipv4_header_1;
    @name(".validate_outer_ipv6_header") validate_outer_ipv6_header() validate_outer_ipv6_header_1;
    @name(".validate_mpls_header") validate_mpls_header() validate_mpls_header_1;
    apply {
        switch (validate_outer_ethernet_0.apply().action_run) {
            default: {
                if (hdr.ipv4.isValid()) 
                    validate_outer_ipv4_header_1.apply(hdr, meta, standard_metadata);
                else 
                    if (hdr.ipv6.isValid()) 
                        validate_outer_ipv6_header_1.apply(hdr, meta, standard_metadata);
                    else 
                        if (hdr.mpls[0].isValid()) 
                            validate_mpls_header_1.apply(hdr, meta, standard_metadata);
            }
            malformed_outer_ethernet_packet_0: {
            }
        }

    }
}

control process_storm_control(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".storm_control_meter") meter(32w1024, MeterType.bytes) storm_control_meter_0;
    @name(".nop") action nop_10() {
    }
    @name(".set_storm_control_meter") action set_storm_control_meter_0(bit<32> meter_idx) {
        storm_control_meter_0.execute_meter<bit<1>>(meter_idx, meta.security_metadata.storm_control_color);
    }
    @name(".storm_control") table storm_control_0 {
        actions = {
            nop_10();
            set_storm_control_meter_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.ifindex: exact @name("ingress_metadata.ifindex") ;
            meta.l2_metadata.lkp_pkt_type: ternary @name("l2_metadata.lkp_pkt_type") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        storm_control_0.apply();
    }
}

control process_port_vlan_mapping(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_bd") action set_bd_0(bit<16> bd, bit<12> vrf, bit<10> rmac_group, bit<1> ipv4_unicast_enabled, bit<1> ipv6_unicast_enabled, bit<2> ipv4_urpf_mode, bit<2> ipv6_urpf_mode, bit<1> igmp_snooping_enabled, bit<1> mld_snooping_enabled, bit<16> bd_label, bit<10> stp_group, bit<16> stats_idx, bit<1> learning_enabled) {
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
    @name(".port_vlan_mapping_miss") action port_vlan_mapping_miss_0() {
        meta.l2_metadata.port_vlan_mapping_miss = 1w1;
    }
    @name(".port_vlan_mapping") table port_vlan_mapping_0 {
        actions = {
            set_bd_0();
            port_vlan_mapping_miss_0();
            @defaultonly NoAction();
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
        default_action = NoAction();
    }
    apply {
        port_vlan_mapping_0.apply();
    }
}

control process_spanning_tree(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_stp_state") action set_stp_state_0(bit<3> stp_state) {
        meta.l2_metadata.stp_state = stp_state;
    }
    @name(".spanning_tree") table spanning_tree_0 {
        actions = {
            set_stp_state_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.ifindex: exact @name("ingress_metadata.ifindex") ;
            meta.l2_metadata.stp_group   : exact @name("l2_metadata.stp_group") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        if (meta.l2_metadata.stp_group != 10w0) 
            spanning_tree_0.apply();
    }
}

control process_ip_sourceguard(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss_0() {
    }
    @name(".ipsg_miss") action ipsg_miss_0() {
        meta.security_metadata.ipsg_check_fail = 1w1;
    }
    @name(".ipsg") table ipsg_0 {
        actions = {
            on_miss_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.ifindex : exact @name("ingress_metadata.ifindex") ;
            meta.ingress_metadata.bd      : exact @name("ingress_metadata.bd") ;
            meta.l2_metadata.lkp_mac_sa   : exact @name("l2_metadata.lkp_mac_sa") ;
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("ipv4_metadata.lkp_ipv4_sa") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".ipsg_permit_special") table ipsg_permit_special_0 {
        actions = {
            ipsg_miss_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.lkp_ip_proto : ternary @name("l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("l3_metadata.lkp_l4_dport") ;
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("ipv4_metadata.lkp_ipv4_da") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        if (meta.security_metadata.ipsg_enabled == 1w1) 
            switch (ipsg_0.apply().action_run) {
                on_miss_0: {
                    ipsg_permit_special_0.apply();
                }
            }

    }
}

control process_int_endpoint(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control process_ipv4_vtep(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_11() {
    }
    @name(".set_tunnel_termination_flag") action set_tunnel_termination_flag_0() {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".on_miss") action on_miss_1() {
    }
    @name(".src_vtep_hit") action src_vtep_hit_0(bit<16> ifindex) {
        meta.ingress_metadata.ifindex = ifindex;
    }
    @name(".ipv4_dest_vtep") table ipv4_dest_vtep_0 {
        actions = {
            nop_11();
            set_tunnel_termination_flag_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf                    : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_da          : exact @name("ipv4_metadata.lkp_ipv4_da") ;
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".ipv4_src_vtep") table ipv4_src_vtep_0 {
        actions = {
            on_miss_1();
            src_vtep_hit_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("ipv4_metadata.lkp_ipv4_sa") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        switch (ipv4_src_vtep_0.apply().action_run) {
            src_vtep_hit_0: {
                ipv4_dest_vtep_0.apply();
            }
        }

    }
}

control process_ipv6_vtep(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_12() {
    }
    @name(".set_tunnel_termination_flag") action set_tunnel_termination_flag_1() {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
    }
    @name(".on_miss") action on_miss_2() {
    }
    @name(".src_vtep_hit") action src_vtep_hit_1(bit<16> ifindex) {
        meta.ingress_metadata.ifindex = ifindex;
    }
    @name(".ipv6_dest_vtep") table ipv6_dest_vtep_0 {
        actions = {
            nop_12();
            set_tunnel_termination_flag_1();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf                    : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_da          : exact @name("ipv6_metadata.lkp_ipv6_da") ;
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".ipv6_src_vtep") table ipv6_src_vtep_0 {
        actions = {
            on_miss_2();
            src_vtep_hit_1();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_sa: exact @name("ipv6_metadata.lkp_ipv6_sa") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        switch (ipv6_src_vtep_0.apply().action_run) {
            src_vtep_hit_1: {
                ipv6_dest_vtep_0.apply();
            }
        }

    }
}

control process_mpls(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".terminate_eompls") action terminate_eompls_0(bit<16> bd, bit<5> tunnel_type) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type;
        meta.ingress_metadata.bd = bd;
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_vpls") action terminate_vpls_0(bit<16> bd, bit<5> tunnel_type) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.tunnel_metadata.ingress_tunnel_type = tunnel_type;
        meta.ingress_metadata.bd = bd;
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_ipv4_over_mpls") action terminate_ipv4_over_mpls_0(bit<12> vrf, bit<5> tunnel_type) {
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
    @name(".terminate_ipv6_over_mpls") action terminate_ipv6_over_mpls_0(bit<12> vrf, bit<5> tunnel_type) {
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
    @name(".terminate_pw") action terminate_pw_0(bit<16> ifindex) {
        meta.ingress_metadata.egress_ifindex = ifindex;
    }
    @name(".forward_mpls") action forward_mpls_0(bit<16> nexthop_index) {
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
        meta.l3_metadata.fib_hit = 1w1;
    }
    @name(".mpls") table mpls_1 {
        actions = {
            terminate_eompls_0();
            terminate_vpls_0();
            terminate_ipv4_over_mpls_0();
            terminate_ipv6_over_mpls_0();
            terminate_pw_0();
            forward_mpls_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.mpls_label: exact @name("tunnel_metadata.mpls_label") ;
            hdr.inner_ipv4.isValid()       : exact @name("inner_ipv4.$valid$") ;
            hdr.inner_ipv6.isValid()       : exact @name("inner_ipv6.$valid$") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        mpls_1.apply();
    }
}

control process_tunnel(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss_3() {
    }
    @name(".outer_rmac_hit") action outer_rmac_hit_0() {
        meta.l3_metadata.rmac_hit = 1w1;
    }
    @name(".nop") action nop_13() {
    }
    @name(".terminate_tunnel_inner_non_ip") action terminate_tunnel_inner_non_ip_0(bit<16> bd, bit<16> bd_label, bit<16> stats_idx) {
        meta.tunnel_metadata.tunnel_terminate = 1w1;
        meta.ingress_metadata.bd = bd;
        meta.l3_metadata.lkp_ip_type = 2w0;
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.acl_metadata.bd_label = bd_label;
        meta.l2_metadata.bd_stats_idx = stats_idx;
    }
    @name(".terminate_tunnel_inner_ethernet_ipv4") action terminate_tunnel_inner_ethernet_ipv4_0(bit<16> bd, bit<12> vrf, bit<10> rmac_group, bit<16> bd_label, bit<1> ipv4_unicast_enabled, bit<2> ipv4_urpf_mode, bit<1> igmp_snooping_enabled, bit<16> stats_idx) {
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
    @name(".terminate_tunnel_inner_ipv4") action terminate_tunnel_inner_ipv4_0(bit<12> vrf, bit<10> rmac_group, bit<2> ipv4_urpf_mode, bit<1> ipv4_unicast_enabled) {
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
    @name(".terminate_tunnel_inner_ethernet_ipv6") action terminate_tunnel_inner_ethernet_ipv6_0(bit<16> bd, bit<12> vrf, bit<10> rmac_group, bit<16> bd_label, bit<1> ipv6_unicast_enabled, bit<2> ipv6_urpf_mode, bit<1> mld_snooping_enabled, bit<16> stats_idx) {
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
    @name(".terminate_tunnel_inner_ipv6") action terminate_tunnel_inner_ipv6_0(bit<12> vrf, bit<10> rmac_group, bit<1> ipv6_unicast_enabled, bit<2> ipv6_urpf_mode) {
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
    @name(".outer_rmac") table outer_rmac_0 {
        actions = {
            on_miss_3();
            outer_rmac_hit_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.rmac_group: exact @name("l3_metadata.rmac_group") ;
            meta.l2_metadata.lkp_mac_da: exact @name("l2_metadata.lkp_mac_da") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunnel") table tunnel_0 {
        actions = {
            nop_13();
            terminate_tunnel_inner_non_ip_0();
            terminate_tunnel_inner_ethernet_ipv4_0();
            terminate_tunnel_inner_ipv4_0();
            terminate_tunnel_inner_ethernet_ipv6_0();
            terminate_tunnel_inner_ipv6_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.tunnel_vni         : exact @name("tunnel_metadata.tunnel_vni") ;
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type") ;
            hdr.inner_ipv4.isValid()                : exact @name("inner_ipv4.$valid$") ;
            hdr.inner_ipv6.isValid()                : exact @name("inner_ipv6.$valid$") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".process_ipv4_vtep") process_ipv4_vtep() process_ipv4_vtep_1;
    @name(".process_ipv6_vtep") process_ipv6_vtep() process_ipv6_vtep_1;
    @name(".process_mpls") process_mpls() process_mpls_1;
    apply {
        switch (outer_rmac_0.apply().action_run) {
            outer_rmac_hit_0: {
                if (hdr.ipv4.isValid()) 
                    process_ipv4_vtep_1.apply(hdr, meta, standard_metadata);
                else 
                    if (hdr.ipv6.isValid()) 
                        process_ipv6_vtep_1.apply(hdr, meta, standard_metadata);
                    else 
                        if (hdr.mpls[0].isValid()) 
                            process_mpls_1.apply(hdr, meta, standard_metadata);
            }
        }

        if (meta.tunnel_metadata.tunnel_terminate == 1w1) 
            tunnel_0.apply();
    }
}

control process_validate_packet(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_14() {
    }
    @name(".set_unicast") action set_unicast_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
    }
    @name(".set_unicast_and_ipv6_src_is_link_local") action set_unicast_and_ipv6_src_is_link_local_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.ipv6_metadata.ipv6_src_is_link_local = 1w1;
    }
    @name(".set_multicast") action set_multicast_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w1;
    }
    @name(".set_multicast_and_ipv6_src_is_link_local") action set_multicast_and_ipv6_src_is_link_local_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.ipv6_metadata.ipv6_src_is_link_local = 1w1;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w1;
    }
    @name(".set_broadcast") action set_broadcast_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.bd_stats_idx = meta.l2_metadata.bd_stats_idx + 16w2;
    }
    @name(".set_malformed_packet") action set_malformed_packet_0(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
    }
    @name(".validate_packet") table validate_packet_0 {
        actions = {
            nop_14();
            set_unicast_0();
            set_unicast_and_ipv6_src_is_link_local_0();
            set_multicast_0();
            set_multicast_and_ipv6_src_is_link_local_0();
            set_broadcast_0();
            set_malformed_packet_0();
            @defaultonly NoAction();
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
        default_action = NoAction();
    }
    apply {
        if (meta.ingress_metadata.drop_flag == 1w0) 
            validate_packet_0.apply();
    }
}

control process_mac(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_15() {
    }
    @name(".dmac_hit") action dmac_hit_0(bit<16> ifindex) {
        meta.ingress_metadata.egress_ifindex = ifindex;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex;
    }
    @name(".dmac_multicast_hit") action dmac_multicast_hit_0(bit<16> mc_index) {
        meta.intrinsic_metadata.mcast_grp = mc_index;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".dmac_miss") action dmac_miss_0() {
        meta.ingress_metadata.egress_ifindex = 16w65535;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".dmac_redirect_nexthop") action dmac_redirect_nexthop_0(bit<16> nexthop_index) {
        meta.l2_metadata.l2_redirect = 1w1;
        meta.l2_metadata.l2_nexthop = nexthop_index;
        meta.l2_metadata.l2_nexthop_type = 1w0;
    }
    @name(".dmac_redirect_ecmp") action dmac_redirect_ecmp_0(bit<16> ecmp_index) {
        meta.l2_metadata.l2_redirect = 1w1;
        meta.l2_metadata.l2_nexthop = ecmp_index;
        meta.l2_metadata.l2_nexthop_type = 1w1;
    }
    @name(".dmac_drop") action dmac_drop_0() {
        mark_to_drop();
    }
    @name(".smac_miss") action smac_miss_0() {
        meta.l2_metadata.l2_src_miss = 1w1;
    }
    @name(".smac_hit") action smac_hit_0(bit<16> ifindex) {
        meta.l2_metadata.l2_src_move = meta.ingress_metadata.ifindex ^ ifindex;
    }
    @name(".dmac") table dmac_0 {
        support_timeout = true;
        actions = {
            nop_15();
            dmac_hit_0();
            dmac_multicast_hit_0();
            dmac_miss_0();
            dmac_redirect_nexthop_0();
            dmac_redirect_ecmp_0();
            dmac_drop_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.bd   : exact @name("ingress_metadata.bd") ;
            meta.l2_metadata.lkp_mac_da: exact @name("l2_metadata.lkp_mac_da") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".smac") table smac_0 {
        actions = {
            nop_15();
            smac_miss_0();
            smac_hit_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.bd   : exact @name("ingress_metadata.bd") ;
            meta.l2_metadata.lkp_mac_sa: exact @name("l2_metadata.lkp_mac_sa") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        smac_0.apply();
        dmac_0.apply();
    }
}

control process_mac_acl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_16() {
    }
    @name(".acl_log") action acl_log_0(bit<16> acl_stats_index) {
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_deny") action acl_deny_0(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_deny = 1w1;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_permit") action acl_permit_0(bit<16> acl_stats_index) {
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_mirror") action acl_mirror_0(bit<32> session_id, bit<16> acl_stats_index) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.enable_dod = 1w0;
        clone3<tuple<bit<32>, bit<16>>>(CloneType.I2E, session_id, { meta.i2e_metadata.ingress_tstamp, meta.i2e_metadata.mirror_session_id });
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".mac_acl") table mac_acl_0 {
        actions = {
            nop_16();
            acl_log_0();
            acl_deny_0();
            acl_permit_0();
            acl_mirror_0();
            @defaultonly NoAction();
        }
        key = {
            meta.acl_metadata.if_label   : ternary @name("acl_metadata.if_label") ;
            meta.acl_metadata.bd_label   : ternary @name("acl_metadata.bd_label") ;
            meta.l2_metadata.lkp_mac_sa  : ternary @name("l2_metadata.lkp_mac_sa") ;
            meta.l2_metadata.lkp_mac_da  : ternary @name("l2_metadata.lkp_mac_da") ;
            meta.l2_metadata.lkp_mac_type: ternary @name("l2_metadata.lkp_mac_type") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        mac_acl_0.apply();
    }
}

control process_ip_acl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_17() {
    }
    @name(".acl_log") action acl_log_1(bit<16> acl_stats_index) {
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_deny") action acl_deny_1(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_deny = 1w1;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_permit") action acl_permit_1(bit<16> acl_stats_index) {
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_mirror") action acl_mirror_1(bit<32> session_id, bit<16> acl_stats_index) {
        meta.i2e_metadata.mirror_session_id = (bit<16>)session_id;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.enable_dod = 1w0;
        clone3<tuple<bit<32>, bit<16>>>(CloneType.I2E, session_id, { meta.i2e_metadata.ingress_tstamp, meta.i2e_metadata.mirror_session_id });
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_dod_en") action acl_dod_en_0() {
        meta.ingress_metadata.enable_dod = 1w1;
    }
    @name(".acl_redirect_nexthop") action acl_redirect_nexthop_0(bit<16> nexthop_index, bit<16> acl_stats_index) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = nexthop_index;
        meta.acl_metadata.acl_nexthop_type = 1w0;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".acl_redirect_ecmp") action acl_redirect_ecmp_0(bit<16> ecmp_index, bit<16> acl_stats_index) {
        meta.acl_metadata.acl_redirect = 1w1;
        meta.acl_metadata.acl_nexthop = ecmp_index;
        meta.acl_metadata.acl_nexthop_type = 1w1;
        meta.ingress_metadata.enable_dod = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".ip_acl") table ip_acl_0 {
        actions = {
            nop_17();
            acl_log_1();
            acl_deny_1();
            acl_permit_1();
            acl_mirror_1();
            acl_dod_en_0();
            acl_redirect_nexthop_0();
            acl_redirect_ecmp_0();
            @defaultonly NoAction();
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
        default_action = NoAction();
    }
    @name(".ipv6_acl") table ipv6_acl_0 {
        actions = {
            nop_17();
            acl_log_1();
            acl_deny_1();
            acl_permit_1();
            acl_mirror_1();
            acl_redirect_nexthop_0();
            acl_redirect_ecmp_0();
            @defaultonly NoAction();
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
        default_action = NoAction();
    }
    apply {
        if (meta.l3_metadata.lkp_ip_type == 2w1) 
            ip_acl_0.apply();
        else 
            if (meta.l3_metadata.lkp_ip_type == 2w2) 
                ipv6_acl_0.apply();
    }
}

control process_qos(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_18() {
    }
    @name(".apply_cos_marking") action apply_cos_marking_0(bit<3> cos) {
        meta.qos_metadata.marked_cos = cos;
    }
    @name(".apply_dscp_marking") action apply_dscp_marking_0(bit<8> dscp) {
        meta.qos_metadata.marked_dscp = dscp;
    }
    @name(".apply_tc_marking") action apply_tc_marking_0(bit<3> tc) {
        meta.qos_metadata.marked_exp = tc;
    }
    @name(".qos") table qos_0 {
        actions = {
            nop_18();
            apply_cos_marking_0();
            apply_dscp_marking_0();
            apply_tc_marking_0();
            @defaultonly NoAction();
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
        default_action = NoAction();
    }
    apply {
        qos_0.apply();
    }
}

control process_ipv4_racl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_19() {
    }
    @name(".racl_log") action racl_log_0(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_deny") action racl_deny_0(bit<16> acl_stats_index) {
        meta.acl_metadata.racl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_permit") action racl_permit_0(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_redirect_nexthop") action racl_redirect_nexthop_0(bit<16> nexthop_index, bit<16> acl_stats_index) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = nexthop_index;
        meta.acl_metadata.racl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_redirect_ecmp") action racl_redirect_ecmp_0(bit<16> ecmp_index, bit<16> acl_stats_index) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = ecmp_index;
        meta.acl_metadata.racl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".ipv4_racl") table ipv4_racl_0 {
        actions = {
            nop_19();
            racl_log_0();
            racl_deny_0();
            racl_permit_0();
            racl_redirect_nexthop_0();
            racl_redirect_ecmp_0();
            @defaultonly NoAction();
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
        default_action = NoAction();
    }
    apply {
        ipv4_racl_0.apply();
    }
}

control process_ipv4_urpf(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss_4() {
    }
    @name(".ipv4_urpf_hit") action ipv4_urpf_hit_0(bit<16> urpf_bd_group) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group;
        meta.l3_metadata.urpf_mode = meta.ipv4_metadata.ipv4_urpf_mode;
    }
    @name(".urpf_miss") action urpf_miss_0() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".ipv4_urpf") table ipv4_urpf_0 {
        actions = {
            on_miss_4();
            ipv4_urpf_hit_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("ipv4_metadata.lkp_ipv4_sa") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".ipv4_urpf_lpm") table ipv4_urpf_lpm_0 {
        actions = {
            ipv4_urpf_hit_0();
            urpf_miss_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_sa: lpm @name("ipv4_metadata.lkp_ipv4_sa") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        if (meta.ipv4_metadata.ipv4_urpf_mode != 2w0) 
            switch (ipv4_urpf_0.apply().action_run) {
                on_miss_4: {
                    ipv4_urpf_lpm_0.apply();
                }
            }

    }
}

control process_ipv4_fib(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss_5() {
    }
    @name(".fib_hit_nexthop") action fib_hit_nexthop_0(bit<16> nexthop_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_ecmp") action fib_hit_ecmp_0(bit<16> ecmp_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".ipv4_fib") table ipv4_fib_0 {
        actions = {
            on_miss_5();
            fib_hit_nexthop_0();
            fib_hit_ecmp_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_da: exact @name("ipv4_metadata.lkp_ipv4_da") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".ipv4_fib_lpm") table ipv4_fib_lpm_0 {
        actions = {
            on_miss_5();
            fib_hit_nexthop_0();
            fib_hit_ecmp_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_da: lpm @name("ipv4_metadata.lkp_ipv4_da") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        switch (ipv4_fib_0.apply().action_run) {
            on_miss_5: {
                ipv4_fib_lpm_0.apply();
            }
        }

    }
}

control process_ipv6_racl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_20() {
    }
    @name(".racl_log") action racl_log_1(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_deny") action racl_deny_1(bit<16> acl_stats_index) {
        meta.acl_metadata.racl_deny = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_permit") action racl_permit_1(bit<16> acl_stats_index) {
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_redirect_nexthop") action racl_redirect_nexthop_1(bit<16> nexthop_index, bit<16> acl_stats_index) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = nexthop_index;
        meta.acl_metadata.racl_nexthop_type = 1w0;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".racl_redirect_ecmp") action racl_redirect_ecmp_1(bit<16> ecmp_index, bit<16> acl_stats_index) {
        meta.acl_metadata.racl_redirect = 1w1;
        meta.acl_metadata.racl_nexthop = ecmp_index;
        meta.acl_metadata.racl_nexthop_type = 1w1;
        meta.acl_metadata.acl_stats_index = acl_stats_index;
    }
    @name(".ipv6_racl") table ipv6_racl_0 {
        actions = {
            nop_20();
            racl_log_1();
            racl_deny_1();
            racl_permit_1();
            racl_redirect_nexthop_1();
            racl_redirect_ecmp_1();
            @defaultonly NoAction();
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
        default_action = NoAction();
    }
    apply {
        ipv6_racl_0.apply();
    }
}

control process_ipv6_urpf(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss_6() {
    }
    @name(".ipv6_urpf_hit") action ipv6_urpf_hit_0(bit<16> urpf_bd_group) {
        meta.l3_metadata.urpf_hit = 1w1;
        meta.l3_metadata.urpf_bd_group = urpf_bd_group;
        meta.l3_metadata.urpf_mode = meta.ipv6_metadata.ipv6_urpf_mode;
    }
    @name(".urpf_miss") action urpf_miss_1() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".ipv6_urpf") table ipv6_urpf_0 {
        actions = {
            on_miss_6();
            ipv6_urpf_hit_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_sa: exact @name("ipv6_metadata.lkp_ipv6_sa") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".ipv6_urpf_lpm") table ipv6_urpf_lpm_0 {
        actions = {
            ipv6_urpf_hit_0();
            urpf_miss_1();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_sa: lpm @name("ipv6_metadata.lkp_ipv6_sa") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        if (meta.ipv6_metadata.ipv6_urpf_mode != 2w0) 
            switch (ipv6_urpf_0.apply().action_run) {
                on_miss_6: {
                    ipv6_urpf_lpm_0.apply();
                }
            }

    }
}

control process_ipv6_fib(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss_7() {
    }
    @name(".fib_hit_nexthop") action fib_hit_nexthop_1(bit<16> nexthop_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = nexthop_index;
        meta.l3_metadata.fib_nexthop_type = 1w0;
    }
    @name(".fib_hit_ecmp") action fib_hit_ecmp_1(bit<16> ecmp_index) {
        meta.l3_metadata.fib_hit = 1w1;
        meta.l3_metadata.fib_nexthop = ecmp_index;
        meta.l3_metadata.fib_nexthop_type = 1w1;
    }
    @name(".ipv6_fib") table ipv6_fib_0 {
        actions = {
            on_miss_7();
            fib_hit_nexthop_1();
            fib_hit_ecmp_1();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_da: exact @name("ipv6_metadata.lkp_ipv6_da") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".ipv6_fib_lpm") table ipv6_fib_lpm_0 {
        actions = {
            on_miss_7();
            fib_hit_nexthop_1();
            fib_hit_ecmp_1();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf          : exact @name("l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_da: lpm @name("ipv6_metadata.lkp_ipv6_da") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        switch (ipv6_fib_0.apply().action_run) {
            on_miss_7: {
                ipv6_fib_lpm_0.apply();
            }
        }

    }
}

control process_urpf_bd(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_21() {
    }
    @name(".urpf_bd_miss") action urpf_bd_miss_0() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".urpf_bd") table urpf_bd_0 {
        actions = {
            nop_21();
            urpf_bd_miss_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.urpf_bd_group: exact @name("l3_metadata.urpf_bd_group") ;
            meta.ingress_metadata.bd      : exact @name("ingress_metadata.bd") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        if (meta.l3_metadata.urpf_mode == 2w2 && meta.l3_metadata.urpf_hit == 1w1) 
            urpf_bd_0.apply();
    }
}

control process_ingress_fabric(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_22() {
    }
    @name(".terminate_cpu_packet") action terminate_cpu_packet_0() {
        standard_metadata.egress_spec = (bit<9>)hdr.fabric_header.dstPortOrGroup;
        meta.egress_metadata.bypass = hdr.fabric_header_cpu.txBypass;
        hdr.ethernet.etherType = hdr.fabric_payload_header.etherType;
        hdr.fabric_header.setInvalid();
        hdr.fabric_header_cpu.setInvalid();
        hdr.fabric_payload_header.setInvalid();
    }
    @name(".switch_fabric_unicast_packet") action switch_fabric_unicast_packet_0() {
        meta.fabric_metadata.fabric_header_present = 1w1;
        meta.fabric_metadata.dst_device = hdr.fabric_header.dstDevice;
        meta.fabric_metadata.dst_port = hdr.fabric_header.dstPortOrGroup;
    }
    @name(".terminate_fabric_unicast_packet") action terminate_fabric_unicast_packet_0() {
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
    @name(".switch_fabric_multicast_packet") action switch_fabric_multicast_packet_0() {
        meta.fabric_metadata.fabric_header_present = 1w1;
        meta.intrinsic_metadata.mcast_grp = hdr.fabric_header.dstPortOrGroup;
    }
    @name(".terminate_fabric_multicast_packet") action terminate_fabric_multicast_packet_0() {
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
    @name(".set_ingress_ifindex_properties") action set_ingress_ifindex_properties_0() {
    }
    @name(".terminate_inner_ethernet_non_ip_over_fabric") action terminate_inner_ethernet_non_ip_over_fabric_0() {
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
    }
    @name(".terminate_inner_ethernet_ipv4_over_fabric") action terminate_inner_ethernet_ipv4_over_fabric_0() {
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.inner_ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.inner_ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv4.protocol;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
    }
    @name(".terminate_inner_ipv4_over_fabric") action terminate_inner_ipv4_over_fabric_0() {
        meta.ipv4_metadata.lkp_ipv4_sa = hdr.inner_ipv4.srcAddr;
        meta.ipv4_metadata.lkp_ipv4_da = hdr.inner_ipv4.dstAddr;
        meta.l3_metadata.lkp_ip_version = hdr.inner_ipv4.version;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv4.protocol;
        meta.l3_metadata.lkp_ip_ttl = hdr.inner_ipv4.ttl;
        meta.l3_metadata.lkp_ip_tc = hdr.inner_ipv4.diffserv;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
    }
    @name(".terminate_inner_ethernet_ipv6_over_fabric") action terminate_inner_ethernet_ipv6_over_fabric_0() {
        meta.l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        meta.l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        meta.l2_metadata.lkp_mac_type = hdr.inner_ethernet.etherType;
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv6.nextHdr;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
    }
    @name(".terminate_inner_ipv6_over_fabric") action terminate_inner_ipv6_over_fabric_0() {
        meta.ipv6_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        meta.ipv6_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        meta.l3_metadata.lkp_ip_proto = hdr.inner_ipv6.nextHdr;
        meta.l3_metadata.lkp_l4_sport = meta.l3_metadata.lkp_inner_l4_sport;
        meta.l3_metadata.lkp_l4_dport = meta.l3_metadata.lkp_inner_l4_dport;
    }
    @name(".fabric_ingress_dst_lkp") table fabric_ingress_dst_lkp_0 {
        actions = {
            nop_22();
            terminate_cpu_packet_0();
            switch_fabric_unicast_packet_0();
            terminate_fabric_unicast_packet_0();
            switch_fabric_multicast_packet_0();
            terminate_fabric_multicast_packet_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.fabric_header.dstDevice: exact @name("fabric_header.dstDevice") ;
        }
        default_action = NoAction();
    }
    @name(".fabric_ingress_src_lkp") table fabric_ingress_src_lkp_0 {
        actions = {
            nop_22();
            set_ingress_ifindex_properties_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.fabric_header_multicast.ingressIfindex: exact @name("fabric_header_multicast.ingressIfindex") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunneled_packet_over_fabric") table tunneled_packet_over_fabric_0 {
        actions = {
            terminate_inner_ethernet_non_ip_over_fabric_0();
            terminate_inner_ethernet_ipv4_over_fabric_0();
            terminate_inner_ipv4_over_fabric_0();
            terminate_inner_ethernet_ipv6_over_fabric_0();
            terminate_inner_ipv6_over_fabric_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("tunnel_metadata.ingress_tunnel_type") ;
            hdr.inner_ipv4.isValid()                : exact @name("inner_ipv4.$valid$") ;
            hdr.inner_ipv6.isValid()                : exact @name("inner_ipv6.$valid$") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        fabric_ingress_dst_lkp_0.apply();
        if (hdr.fabric_header_multicast.isValid()) 
            fabric_ingress_src_lkp_0.apply();
        if (meta.tunnel_metadata.tunnel_terminate == 1w1) 
            tunneled_packet_over_fabric_0.apply();
    }
}

control process_hashes(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".compute_lkp_ipv4_hash") action compute_lkp_ipv4_hash_0() {
        hash<bit<16>, bit<16>, tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>>, bit<32>>(meta.hash_metadata.hash1, HashAlgorithm.crc16, 16w0, { meta.ipv4_metadata.lkp_ipv4_sa, meta.ipv4_metadata.lkp_ipv4_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
        hash<bit<16>, bit<16>, tuple<bit<48>, bit<48>, bit<32>, bit<32>, bit<8>, bit<16>, bit<16>>, bit<32>>(meta.hash_metadata.hash2, HashAlgorithm.crc16, 16w0, { meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.ipv4_metadata.lkp_ipv4_sa, meta.ipv4_metadata.lkp_ipv4_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
    }
    @name(".compute_lkp_ipv6_hash") action compute_lkp_ipv6_hash_0() {
        hash<bit<16>, bit<16>, tuple<bit<128>, bit<128>, bit<8>, bit<16>, bit<16>>, bit<32>>(meta.hash_metadata.hash1, HashAlgorithm.crc16, 16w0, { meta.ipv6_metadata.lkp_ipv6_sa, meta.ipv6_metadata.lkp_ipv6_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
        hash<bit<16>, bit<16>, tuple<bit<48>, bit<48>, bit<128>, bit<128>, bit<8>, bit<16>, bit<16>>, bit<32>>(meta.hash_metadata.hash2, HashAlgorithm.crc16, 16w0, { meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.ipv6_metadata.lkp_ipv6_sa, meta.ipv6_metadata.lkp_ipv6_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport }, 32w65536);
    }
    @name(".compute_lkp_non_ip_hash") action compute_lkp_non_ip_hash_0() {
        hash<bit<16>, bit<16>, tuple<bit<16>, bit<48>, bit<48>, bit<16>>, bit<32>>(meta.hash_metadata.hash2, HashAlgorithm.crc16, 16w0, { meta.ingress_metadata.ifindex, meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.l2_metadata.lkp_mac_type }, 32w65536);
    }
    @name(".computed_two_hashes") action computed_two_hashes_0() {
        meta.intrinsic_metadata.mcast_hash = (bit<13>)meta.hash_metadata.hash1;
        meta.hash_metadata.entropy_hash = meta.hash_metadata.hash2;
    }
    @name(".computed_one_hash") action computed_one_hash_0() {
        meta.hash_metadata.hash1 = meta.hash_metadata.hash2;
        meta.intrinsic_metadata.mcast_hash = (bit<13>)meta.hash_metadata.hash2;
        meta.hash_metadata.entropy_hash = meta.hash_metadata.hash2;
    }
    @name(".compute_ipv4_hashes") table compute_ipv4_hashes_0 {
        actions = {
            compute_lkp_ipv4_hash_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("ingress_metadata.drop_flag") ;
        }
        default_action = NoAction();
    }
    @name(".compute_ipv6_hashes") table compute_ipv6_hashes_0 {
        actions = {
            compute_lkp_ipv6_hash_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("ingress_metadata.drop_flag") ;
        }
        default_action = NoAction();
    }
    @name(".compute_non_ip_hashes") table compute_non_ip_hashes_0 {
        actions = {
            compute_lkp_non_ip_hash_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("ingress_metadata.drop_flag") ;
        }
        default_action = NoAction();
    }
    @name(".compute_other_hashes") table compute_other_hashes_0 {
        actions = {
            computed_two_hashes_0();
            computed_one_hash_0();
            @defaultonly NoAction();
        }
        key = {
            meta.hash_metadata.hash1: exact @name("hash_metadata.hash1") ;
        }
        default_action = NoAction();
    }
    apply {
        if (meta.tunnel_metadata.tunnel_terminate == 1w0 && hdr.ipv4.isValid() || meta.tunnel_metadata.tunnel_terminate == 1w1 && hdr.inner_ipv4.isValid()) 
            compute_ipv4_hashes_0.apply();
        else 
            if (meta.tunnel_metadata.tunnel_terminate == 1w0 && hdr.ipv6.isValid() || meta.tunnel_metadata.tunnel_terminate == 1w1 && hdr.inner_ipv6.isValid()) 
                compute_ipv6_hashes_0.apply();
            else 
                compute_non_ip_hashes_0.apply();
        compute_other_hashes_0.apply();
    }
}

control process_ingress_bd_stats(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".ingress_bd_stats_count") @min_width(32) counter(32w1024, CounterType.packets_and_bytes) ingress_bd_stats_count_0;
    @name(".update_ingress_bd_stats") action update_ingress_bd_stats_0() {
        ingress_bd_stats_count_0.count((bit<32>)meta.l2_metadata.bd_stats_idx);
    }
    @name(".ingress_bd_stats") table ingress_bd_stats_0 {
        actions = {
            update_ingress_bd_stats_0();
            @defaultonly NoAction();
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        ingress_bd_stats_0.apply();
    }
}

control process_ingress_acl_stats(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".acl_stats_count") counter(32w1024, CounterType.packets_and_bytes) acl_stats_count_0;
    @name(".acl_stats_update") action acl_stats_update_0() {
        acl_stats_count_0.count((bit<32>)meta.acl_metadata.acl_stats_index);
    }
    @name(".acl_stats") table acl_stats_0 {
        actions = {
            acl_stats_update_0();
            @defaultonly NoAction();
        }
        key = {
            meta.acl_metadata.acl_stats_index: exact @name("acl_metadata.acl_stats_index") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        acl_stats_0.apply();
    }
}

control process_fwd_results(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_23() {
    }
    @name(".set_l2_redirect_action") action set_l2_redirect_action_0() {
        meta.l3_metadata.nexthop_index = meta.l2_metadata.l2_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.l2_metadata.l2_nexthop_type;
    }
    @name(".set_fib_redirect_action") action set_fib_redirect_action_0() {
        meta.l3_metadata.nexthop_index = meta.l3_metadata.fib_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.l3_metadata.fib_nexthop_type;
        meta.l3_metadata.routed = 1w1;
        meta.intrinsic_metadata.mcast_grp = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_cpu_redirect_action") action set_cpu_redirect_action_0() {
        meta.l3_metadata.routed = 1w0;
        meta.intrinsic_metadata.mcast_grp = 16w0;
        standard_metadata.egress_spec = 9w64;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".set_acl_redirect_action") action set_acl_redirect_action_0() {
        meta.l3_metadata.nexthop_index = meta.acl_metadata.acl_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.acl_metadata.acl_nexthop_type;
    }
    @name(".set_racl_redirect_action") action set_racl_redirect_action_0() {
        meta.l3_metadata.nexthop_index = meta.acl_metadata.racl_nexthop;
        meta.nexthop_metadata.nexthop_type = meta.acl_metadata.racl_nexthop_type;
        meta.l3_metadata.routed = 1w1;
    }
    @name(".fwd_result") table fwd_result_0 {
        actions = {
            nop_23();
            set_l2_redirect_action_0();
            set_fib_redirect_action_0();
            set_cpu_redirect_action_0();
            set_acl_redirect_action_0();
            set_racl_redirect_action_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l2_metadata.l2_redirect   : ternary @name("l2_metadata.l2_redirect") ;
            meta.acl_metadata.acl_redirect : ternary @name("acl_metadata.acl_redirect") ;
            meta.acl_metadata.racl_redirect: ternary @name("acl_metadata.racl_redirect") ;
            meta.l3_metadata.rmac_hit      : ternary @name("l3_metadata.rmac_hit") ;
            meta.l3_metadata.fib_hit       : ternary @name("l3_metadata.fib_hit") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        fwd_result_0.apply();
    }
}

control process_nexthop(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_24() {
    }
    @name(".set_ecmp_nexthop_details") action set_ecmp_nexthop_details_0(bit<16> ifindex, bit<16> bd, bit<16> nhop_index, bit<1> tunnel) {
        meta.ingress_metadata.egress_ifindex = ifindex;
        meta.l3_metadata.nexthop_index = nhop_index;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex;
        meta.tunnel_metadata.tunnel_if_check = meta.tunnel_metadata.tunnel_terminate ^ tunnel;
    }
    @name(".set_ecmp_nexthop_details_for_post_routed_flood") action set_ecmp_nexthop_details_for_post_routed_flood_0(bit<16> bd, bit<16> uuc_mc_index, bit<16> nhop_index) {
        meta.intrinsic_metadata.mcast_grp = uuc_mc_index;
        meta.l3_metadata.nexthop_index = nhop_index;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".set_nexthop_details") action set_nexthop_details_0(bit<16> ifindex, bit<16> bd, bit<1> tunnel) {
        meta.ingress_metadata.egress_ifindex = ifindex;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.l2_metadata.same_if_check = meta.l2_metadata.same_if_check ^ ifindex;
        meta.tunnel_metadata.tunnel_if_check = meta.tunnel_metadata.tunnel_terminate ^ tunnel;
    }
    @name(".set_nexthop_details_for_post_routed_flood") action set_nexthop_details_for_post_routed_flood_0(bit<16> bd, bit<16> uuc_mc_index) {
        meta.intrinsic_metadata.mcast_grp = uuc_mc_index;
        meta.ingress_metadata.egress_ifindex = 16w0;
        meta.l3_metadata.same_bd_check = meta.ingress_metadata.bd ^ bd;
        meta.fabric_metadata.dst_device = 8w127;
    }
    @name(".ecmp_group") table ecmp_group_0 {
        actions = {
            nop_24();
            set_ecmp_nexthop_details_0();
            set_ecmp_nexthop_details_for_post_routed_flood_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("l3_metadata.nexthop_index") ;
            meta.hash_metadata.hash1      : selector @name("hash_metadata.hash1") ;
        }
        size = 1024;
        implementation = ecmp_action_profile;
        default_action = NoAction();
    }
    @name(".nexthop") table nexthop_0 {
        actions = {
            nop_24();
            set_nexthop_details_0();
            set_nexthop_details_for_post_routed_flood_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("l3_metadata.nexthop_index") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        if (meta.nexthop_metadata.nexthop_type == 1w1) 
            ecmp_group_0.apply();
        else 
            nexthop_0.apply();
    }
}

control process_multicast_flooding(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_25() {
    }
    @name(".set_bd_flood_mc_index") action set_bd_flood_mc_index_0(bit<16> mc_index) {
        meta.intrinsic_metadata.mcast_grp = mc_index;
    }
    @name(".bd_flood") table bd_flood_0 {
        actions = {
            nop_25();
            set_bd_flood_mc_index_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.bd     : exact @name("ingress_metadata.bd") ;
            meta.l2_metadata.lkp_pkt_type: exact @name("l2_metadata.lkp_pkt_type") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        bd_flood_0.apply();
    }
}

control process_lag(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_lag_miss") action set_lag_miss_0() {
    }
    @name(".set_lag_port") action set_lag_port_0(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".set_lag_remote_port") action set_lag_remote_port_0(bit<8> device, bit<16> port) {
        meta.fabric_metadata.dst_device = device;
        meta.fabric_metadata.dst_port = port;
    }
    @name(".lag_group") table lag_group_0 {
        actions = {
            set_lag_miss_0();
            set_lag_port_0();
            set_lag_remote_port_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.egress_ifindex: exact @name("ingress_metadata.egress_ifindex") ;
            meta.hash_metadata.hash2            : selector @name("hash_metadata.hash2") ;
        }
        size = 1024;
        implementation = lag_action_profile;
        default_action = NoAction();
    }
    apply {
        lag_group_0.apply();
    }
}

@name("mac_learn_digest") struct mac_learn_digest {
    bit<16> bd;
    bit<48> lkp_mac_sa;
    bit<16> ifindex;
}

control process_mac_learning(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_26() {
    }
    @name(".generate_learn_notify") action generate_learn_notify_0() {
        digest<mac_learn_digest>(32w1024, { meta.ingress_metadata.bd, meta.l2_metadata.lkp_mac_sa, meta.ingress_metadata.ifindex });
    }
    @name(".learn_notify") table learn_notify_0 {
        actions = {
            nop_26();
            generate_learn_notify_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l2_metadata.l2_src_miss: ternary @name("l2_metadata.l2_src_miss") ;
            meta.l2_metadata.l2_src_move: ternary @name("l2_metadata.l2_src_move") ;
            meta.l2_metadata.stp_state  : ternary @name("l2_metadata.stp_state") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        if (meta.l2_metadata.learning_enabled == 1w1) 
            learn_notify_0.apply();
    }
}

control process_fabric_lag(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop_27() {
    }
    @name(".set_fabric_lag_port") action set_fabric_lag_port_0(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".set_fabric_multicast") action set_fabric_multicast_0(bit<8> fabric_mgid) {
        meta.multicast_metadata.mcast_grp = meta.intrinsic_metadata.mcast_grp;
    }
    @name(".fabric_lag") table fabric_lag_0 {
        actions = {
            nop_27();
            set_fabric_lag_port_0();
            set_fabric_multicast_0();
            @defaultonly NoAction();
        }
        key = {
            meta.fabric_metadata.dst_device: exact @name("fabric_metadata.dst_device") ;
            meta.hash_metadata.hash2       : selector @name("hash_metadata.hash2") ;
        }
        implementation = fabric_lag_action_profile;
        default_action = NoAction();
    }
    apply {
        fabric_lag_0.apply();
    }
}

control process_system_acl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".drop_stats") counter(32w1024, CounterType.packets) drop_stats_1;
    @name(".drop_stats_2") counter(32w1024, CounterType.packets) drop_stats_3;
    @name(".drop_stats_update") action drop_stats_update_0() {
        drop_stats_3.count((bit<32>)meta.ingress_metadata.drop_reason);
    }
    @name(".nop") action nop_28() {
    }
    @name(".copy_to_cpu") action copy_to_cpu_0(bit<16> reason_code) {
        meta.fabric_metadata.reason_code = reason_code;
        clone3<tuple<bit<16>, bit<16>, bit<16>, bit<9>>>(CloneType.I2E, 32w250, { meta.ingress_metadata.bd, meta.ingress_metadata.ifindex, meta.fabric_metadata.reason_code, meta.ingress_metadata.ingress_port });
    }
    @name(".redirect_to_cpu") action redirect_to_cpu_0(bit<16> reason_code) {
        copy_to_cpu_0(reason_code);
        mark_to_drop();
        meta.fabric_metadata.dst_device = 8w0;
    }
    @name(".drop_packet") action drop_packet_0() {
        mark_to_drop();
    }
    @name(".drop_packet_with_reason") action drop_packet_with_reason_0(bit<32> drop_reason) {
        drop_stats_1.count(drop_reason);
        mark_to_drop();
    }
    @name(".negative_mirror") action negative_mirror_0(bit<32> session_id) {
        clone3<tuple<bit<16>, bit<8>>>(CloneType.I2E, session_id, { meta.ingress_metadata.ifindex, meta.ingress_metadata.drop_reason });
        mark_to_drop();
    }
    @name(".deflect_on_drop") action deflect_on_drop_0() {
        meta.intrinsic_metadata.deflect_on_drop = 1w1;
    }
    @name(".congestion_mirror_set") action congestion_mirror_set_0() {
        deflect_on_drop_0();
    }
    @name(".drop_stats") table drop_stats_4 {
        actions = {
            drop_stats_update_0();
            @defaultonly NoAction();
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".system_acl") table system_acl_0 {
        actions = {
            nop_28();
            redirect_to_cpu_0();
            copy_to_cpu_0();
            drop_packet_0();
            drop_packet_with_reason_0();
            negative_mirror_0();
            congestion_mirror_set_0();
            @defaultonly NoAction();
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
        default_action = NoAction();
    }
    apply {
        system_acl_0.apply();
        if (meta.ingress_metadata.drop_flag == 1w1) 
            drop_stats_4.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".rmac_hit") action rmac_hit_0() {
        meta.l3_metadata.rmac_hit = 1w1;
    }
    @name(".rmac_miss") action rmac_miss_0() {
        meta.l3_metadata.rmac_hit = 1w0;
    }
    @name(".rmac") table rmac_0 {
        actions = {
            rmac_hit_0();
            rmac_miss_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.rmac_group: exact @name("l3_metadata.rmac_group") ;
            meta.l2_metadata.lkp_mac_da: exact @name("l2_metadata.lkp_mac_da") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".process_ingress_port_mapping") process_ingress_port_mapping() process_ingress_port_mapping_1;
    @name(".process_validate_outer_header") process_validate_outer_header() process_validate_outer_header_1;
    @name(".process_storm_control") process_storm_control() process_storm_control_1;
    @name(".process_port_vlan_mapping") process_port_vlan_mapping() process_port_vlan_mapping_1;
    @name(".process_spanning_tree") process_spanning_tree() process_spanning_tree_1;
    @name(".process_ip_sourceguard") process_ip_sourceguard() process_ip_sourceguard_1;
    @name(".process_int_endpoint") process_int_endpoint() process_int_endpoint_1;
    @name(".process_tunnel") process_tunnel() process_tunnel_1;
    @name(".process_validate_packet") process_validate_packet() process_validate_packet_1;
    @name(".process_mac") process_mac() process_mac_1;
    @name(".process_mac_acl") process_mac_acl() process_mac_acl_1;
    @name(".process_ip_acl") process_ip_acl() process_ip_acl_1;
    @name(".process_qos") process_qos() process_qos_1;
    @name(".process_ipv4_racl") process_ipv4_racl() process_ipv4_racl_1;
    @name(".process_ipv4_urpf") process_ipv4_urpf() process_ipv4_urpf_1;
    @name(".process_ipv4_fib") process_ipv4_fib() process_ipv4_fib_1;
    @name(".process_ipv6_racl") process_ipv6_racl() process_ipv6_racl_1;
    @name(".process_ipv6_urpf") process_ipv6_urpf() process_ipv6_urpf_1;
    @name(".process_ipv6_fib") process_ipv6_fib() process_ipv6_fib_1;
    @name(".process_urpf_bd") process_urpf_bd() process_urpf_bd_1;
    @name(".process_ingress_fabric") process_ingress_fabric() process_ingress_fabric_1;
    @name(".process_hashes") process_hashes() process_hashes_1;
    @name(".process_ingress_bd_stats") process_ingress_bd_stats() process_ingress_bd_stats_1;
    @name(".process_ingress_acl_stats") process_ingress_acl_stats() process_ingress_acl_stats_1;
    @name(".process_fwd_results") process_fwd_results() process_fwd_results_1;
    @name(".process_nexthop") process_nexthop() process_nexthop_1;
    @name(".process_multicast_flooding") process_multicast_flooding() process_multicast_flooding_1;
    @name(".process_lag") process_lag() process_lag_1;
    @name(".process_mac_learning") process_mac_learning() process_mac_learning_1;
    @name(".process_fabric_lag") process_fabric_lag() process_fabric_lag_1;
    @name(".process_system_acl") process_system_acl() process_system_acl_1;
    apply {
        process_ingress_port_mapping_1.apply(hdr, meta, standard_metadata);
        process_validate_outer_header_1.apply(hdr, meta, standard_metadata);
        if (meta.ingress_metadata.port_type == 2w0) {
            process_storm_control_1.apply(hdr, meta, standard_metadata);
            process_port_vlan_mapping_1.apply(hdr, meta, standard_metadata);
            process_spanning_tree_1.apply(hdr, meta, standard_metadata);
            process_ip_sourceguard_1.apply(hdr, meta, standard_metadata);
            process_int_endpoint_1.apply(hdr, meta, standard_metadata);
            process_tunnel_1.apply(hdr, meta, standard_metadata);
            if (!hdr.mpls[0].isValid() || hdr.mpls[0].isValid() && meta.tunnel_metadata.tunnel_terminate == 1w1) {
                process_validate_packet_1.apply(hdr, meta, standard_metadata);
                process_mac_1.apply(hdr, meta, standard_metadata);
                if (meta.l3_metadata.lkp_ip_type == 2w0) 
                    process_mac_acl_1.apply(hdr, meta, standard_metadata);
                else 
                    process_ip_acl_1.apply(hdr, meta, standard_metadata);
                process_qos_1.apply(hdr, meta, standard_metadata);
                switch (rmac_0.apply().action_run) {
                    rmac_hit_0: {
                        if (meta.l3_metadata.lkp_ip_type == 2w1 && meta.ipv4_metadata.ipv4_unicast_enabled == 1w1) {
                            process_ipv4_racl_1.apply(hdr, meta, standard_metadata);
                            process_ipv4_urpf_1.apply(hdr, meta, standard_metadata);
                            process_ipv4_fib_1.apply(hdr, meta, standard_metadata);
                        }
                        else 
                            if (meta.l3_metadata.lkp_ip_type == 2w2 && meta.ipv6_metadata.ipv6_unicast_enabled == 1w1) {
                                process_ipv6_racl_1.apply(hdr, meta, standard_metadata);
                                process_ipv6_urpf_1.apply(hdr, meta, standard_metadata);
                                process_ipv6_fib_1.apply(hdr, meta, standard_metadata);
                            }
                        process_urpf_bd_1.apply(hdr, meta, standard_metadata);
                    }
                }

            }
        }
        else 
            process_ingress_fabric_1.apply(hdr, meta, standard_metadata);
        process_hashes_1.apply(hdr, meta, standard_metadata);
        if (meta.ingress_metadata.port_type == 2w0) {
            process_ingress_bd_stats_1.apply(hdr, meta, standard_metadata);
            process_ingress_acl_stats_1.apply(hdr, meta, standard_metadata);
            process_fwd_results_1.apply(hdr, meta, standard_metadata);
            process_nexthop_1.apply(hdr, meta, standard_metadata);
            if (meta.ingress_metadata.egress_ifindex == 16w65535) 
                process_multicast_flooding_1.apply(hdr, meta, standard_metadata);
            else 
                process_lag_1.apply(hdr, meta, standard_metadata);
            process_mac_learning_1.apply(hdr, meta, standard_metadata);
        }
        process_fabric_lag_1.apply(hdr, meta, standard_metadata);
        process_system_acl_1.apply(hdr, meta, standard_metadata);
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

