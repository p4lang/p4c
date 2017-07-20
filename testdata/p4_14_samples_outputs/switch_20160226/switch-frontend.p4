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

struct __metadataImpl {
    @name("acl_metadata") 
    acl_metadata_t               acl_metadata;
    @name("egress_filter_metadata") 
    egress_filter_metadata_t     egress_filter_metadata;
    @name("egress_metadata") 
    egress_metadata_t            egress_metadata;
    @name("fabric_metadata") 
    fabric_metadata_t            fabric_metadata;
    @pa_atomic("ingress", "hash_metadata.hash1") @pa_solitary("ingress", "hash_metadata.hash1") @pa_atomic("ingress", "hash_metadata.hash2") @pa_solitary("ingress", "hash_metadata.hash2") @name("hash_metadata") 
    hash_metadata_t              hash_metadata;
    @name("i2e_metadata") 
    i2e_metadata_t               i2e_metadata;
    @name("ingress_metadata") 
    ingress_metadata_t           ingress_metadata;
    @name("int_metadata") 
    int_metadata_t               int_metadata;
    @name("int_metadata_i2e") 
    int_metadata_i2e_t           int_metadata_i2e;
    @name("intrinsic_metadata") 
    ingress_intrinsic_metadata_t intrinsic_metadata;
    @name("ipv4_metadata") 
    ipv4_metadata_t              ipv4_metadata;
    @name("ipv6_metadata") 
    ipv6_metadata_t              ipv6_metadata;
    @name("l2_metadata") 
    l2_metadata_t                l2_metadata;
    @name("l3_metadata") 
    l3_metadata_t                l3_metadata;
    @name("multicast_metadata") 
    multicast_metadata_t         multicast_metadata;
    @name("nexthop_metadata") 
    nexthop_metadata_t           nexthop_metadata;
    @name("qos_metadata") 
    qos_metadata_t               qos_metadata;
    @name("security_metadata") 
    security_metadata_t          security_metadata;
    @name("standard_metadata") 
    standard_metadata_t          standard_metadata;
    @name("tunnel_metadata") 
    tunnel_metadata_t            tunnel_metadata;
}

struct __headersImpl {
    @name("arp_rarp") 
    arp_rarp_t                              arp_rarp;
    @name("arp_rarp_ipv4") 
    arp_rarp_ipv4_t                         arp_rarp_ipv4;
    @name("bfd") 
    bfd_t                                   bfd;
    @name("eompls") 
    eompls_t                                eompls;
    @name("erspan_t3_header") 
    erspan_header_t3_t_0                    erspan_t3_header;
    @name("ethernet") 
    ethernet_t                              ethernet;
    @name("fabric_header") 
    fabric_header_t                         fabric_header;
    @name("fabric_header_cpu") 
    fabric_header_cpu_t                     fabric_header_cpu;
    @name("fabric_header_mirror") 
    fabric_header_mirror_t                  fabric_header_mirror;
    @name("fabric_header_multicast") 
    fabric_header_multicast_t               fabric_header_multicast;
    @name("fabric_header_unicast") 
    fabric_header_unicast_t                 fabric_header_unicast;
    @name("fabric_payload_header") 
    fabric_payload_header_t                 fabric_payload_header;
    @name("fcoe") 
    fcoe_header_t                           fcoe;
    @name("genv") 
    genv_t                                  genv;
    @name("gre") 
    gre_t                                   gre;
    @name("icmp") 
    icmp_t                                  icmp;
    @name("inner_ethernet") 
    ethernet_t                              inner_ethernet;
    @name("inner_icmp") 
    icmp_t                                  inner_icmp;
    @name("inner_ipv4") 
    ipv4_t                                  inner_ipv4;
    @name("inner_ipv6") 
    ipv6_t                                  inner_ipv6;
    @name("inner_sctp") 
    sctp_t                                  inner_sctp;
    @name("inner_tcp") 
    tcp_t                                   inner_tcp;
    @name("inner_udp") 
    udp_t                                   inner_udp;
    @name("int_egress_port_id_header") 
    int_egress_port_id_header_t             int_egress_port_id_header;
    @name("int_egress_port_tx_utilization_header") 
    int_egress_port_tx_utilization_header_t int_egress_port_tx_utilization_header;
    @name("int_header") 
    int_header_t                            int_header;
    @name("int_hop_latency_header") 
    int_hop_latency_header_t                int_hop_latency_header;
    @name("int_ingress_port_id_header") 
    int_ingress_port_id_header_t            int_ingress_port_id_header;
    @name("int_ingress_tstamp_header") 
    int_ingress_tstamp_header_t             int_ingress_tstamp_header;
    @name("int_q_congestion_header") 
    int_q_congestion_header_t               int_q_congestion_header;
    @name("int_q_occupancy_header") 
    int_q_occupancy_header_t                int_q_occupancy_header;
    @name("int_switch_id_header") 
    int_switch_id_header_t                  int_switch_id_header;
    @name("ipv4") 
    ipv4_t                                  ipv4;
    @name("ipv6") 
    ipv6_t                                  ipv6;
    @name("lisp") 
    lisp_t                                  lisp;
    @name("llc_header") 
    llc_header_t                            llc_header;
    @name("nsh") 
    nsh_t                                   nsh;
    @name("nsh_context") 
    nsh_context_t                           nsh_context;
    @name("nvgre") 
    nvgre_t                                 nvgre;
    @name("outer_udp") 
    udp_t                                   outer_udp;
    @name("roce") 
    roce_header_t                           roce;
    @name("roce_v2") 
    roce_v2_header_t                        roce_v2;
    @name("sctp") 
    sctp_t                                  sctp;
    @name("sflow") 
    sflow_t                                 sflow;
    @name("sflow_internal_ethernet") 
    sflow_internal_ethernet_t               sflow_internal_ethernet;
    @name("sflow_record") 
    sflow_record_t                          sflow_record;
    @name("sflow_sample") 
    sflow_sample_t                          sflow_sample;
    @name("snap_header") 
    snap_header_t                           snap_header;
    @name("tcp") 
    tcp_t                                   tcp;
    @name("trill") 
    trill_t                                 trill;
    @name("udp") 
    udp_t                                   udp;
    @name("vntag") 
    vntag_t                                 vntag;
    @name("vxlan") 
    vxlan_t                                 vxlan;
    @name("vxlan_gpe") 
    vxlan_gpe_t                             vxlan_gpe;
    @name("vxlan_gpe_int_header") 
    vxlan_gpe_int_header_t                  vxlan_gpe_int_header;
    @name(".mpls") 
    mpls_t[3]                               mpls;
    @name(".vlan_tag_") 
    vlan_tag_t[2]                           vlan_tag_;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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

control process_ingress_port_mapping(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            meta.standard_metadata.ingress_port: exact @name("meta.standard_metadata.ingress_port") ;
        }
        size = 288;
        default_action = NoAction();
    }
    apply {
        ingress_port_mapping_0.apply();
    }
}

control validate_outer_ipv4_header(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            hdr.ipv4.version                     : ternary @name("hdr.ipv4.version") ;
            meta.l3_metadata.lkp_ip_ttl          : ternary @name("meta.l3_metadata.lkp_ip_ttl") ;
            meta.ipv4_metadata.lkp_ipv4_sa[31:24]: ternary @name("meta.ipv4_metadata.lkp_ipv4_sa[31:24]") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        validate_outer_ipv4_packet_0.apply();
    }
}

control validate_outer_ipv6_header(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            hdr.ipv6.version                       : ternary @name("hdr.ipv6.version") ;
            meta.l3_metadata.lkp_ip_ttl            : ternary @name("meta.l3_metadata.lkp_ip_ttl") ;
            meta.ipv6_metadata.lkp_ipv6_sa[127:112]: ternary @name("meta.ipv6_metadata.lkp_ipv6_sa[127:112]") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        validate_outer_ipv6_packet_0.apply();
    }
}

control validate_mpls_header(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            hdr.mpls[0].label    : ternary @name("hdr..mpls[0].label") ;
            hdr.mpls[0].bos      : ternary @name("hdr..mpls[0].bos") ;
            hdr.mpls[0].isValid(): exact @name("hdr..mpls[0].isValid()") ;
            hdr.mpls[1].label    : ternary @name("hdr..mpls[1].label") ;
            hdr.mpls[1].bos      : ternary @name("hdr..mpls[1].bos") ;
            hdr.mpls[1].isValid(): exact @name("hdr..mpls[1].isValid()") ;
            hdr.mpls[2].label    : ternary @name("hdr..mpls[2].label") ;
            hdr.mpls[2].bos      : ternary @name("hdr..mpls[2].bos") ;
            hdr.mpls[2].isValid(): exact @name("hdr..mpls[2].isValid()") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        validate_mpls_packet_0.apply();
    }
}

control process_validate_outer_header(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".malformed_outer_ethernet_packet") action malformed_outer_ethernet_packet_0(bit<8> drop_reason) {
        meta.ingress_metadata.drop_flag = 1w1;
        meta.ingress_metadata.drop_reason = drop_reason;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_untagged") action set_valid_outer_unicast_packet_untagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_single_tagged") action set_valid_outer_unicast_packet_single_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_double_tagged") action set_valid_outer_unicast_packet_double_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_unicast_packet_qinq_tagged") action set_valid_outer_unicast_packet_qinq_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w1;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_untagged") action set_valid_outer_multicast_packet_untagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_single_tagged") action set_valid_outer_multicast_packet_single_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_double_tagged") action set_valid_outer_multicast_packet_double_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_multicast_packet_qinq_tagged") action set_valid_outer_multicast_packet_qinq_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w2;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_untagged") action set_valid_outer_broadcast_packet_untagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_single_tagged") action set_valid_outer_broadcast_packet_single_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_double_tagged") action set_valid_outer_broadcast_packet_double_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
        meta.l2_metadata.same_if_check = meta.ingress_metadata.ifindex;
    }
    @name(".set_valid_outer_broadcast_packet_qinq_tagged") action set_valid_outer_broadcast_packet_qinq_tagged_0() {
        meta.l2_metadata.lkp_pkt_type = 3w4;
        meta.l2_metadata.lkp_mac_type = hdr.ethernet.etherType;
        meta.standard_metadata.egress_spec = 9w511;
        meta.i2e_metadata.ingress_tstamp = (bit<32>)meta.intrinsic_metadata.ingress_global_tstamp;
        meta.ingress_metadata.ingress_port = meta.standard_metadata.ingress_port;
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
            meta.l2_metadata.lkp_mac_sa: ternary @name("meta.l2_metadata.lkp_mac_sa") ;
            meta.l2_metadata.lkp_mac_da: ternary @name("meta.l2_metadata.lkp_mac_da") ;
            hdr.vlan_tag_[0].isValid() : exact @name("hdr..vlan_tag_[0].isValid()") ;
            hdr.vlan_tag_[1].isValid() : exact @name("hdr..vlan_tag_[1].isValid()") ;
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
                    validate_outer_ipv4_header_1.apply(hdr, meta, __standard_metadata);
                else 
                    if (hdr.ipv6.isValid()) 
                        validate_outer_ipv6_header_1.apply(hdr, meta, __standard_metadata);
                    else 
                        if (hdr.mpls[0].isValid()) 
                            validate_mpls_header_1.apply(hdr, meta, __standard_metadata);
            }
            malformed_outer_ethernet_packet_0: {
            }
        }

    }
}

control process_storm_control(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".storm_control_meter") meter(32w1024, MeterType.bytes) storm_control_meter_0;
    @name(".nop") action nop_0() {
    }
    @name(".set_storm_control_meter") action set_storm_control_meter_0(bit<32> meter_idx) {
        storm_control_meter_0.execute_meter<bit<1>>(meter_idx, meta.security_metadata.storm_control_color);
    }
    @name(".storm_control") table storm_control_0 {
        actions = {
            nop_0();
            set_storm_control_meter_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.ifindex: exact @name("meta.ingress_metadata.ifindex") ;
            meta.l2_metadata.lkp_pkt_type: ternary @name("meta.l2_metadata.lkp_pkt_type") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        storm_control_0.apply();
    }
}

control process_port_vlan_mapping(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            meta.ingress_metadata.ifindex: exact @name("meta.ingress_metadata.ifindex") ;
            hdr.vlan_tag_[0].isValid()   : exact @name("hdr..vlan_tag_[0].isValid()") ;
            hdr.vlan_tag_[0].vid         : exact @name("hdr..vlan_tag_[0].vid") ;
            hdr.vlan_tag_[1].isValid()   : exact @name("hdr..vlan_tag_[1].isValid()") ;
            hdr.vlan_tag_[1].vid         : exact @name("hdr..vlan_tag_[1].vid") ;
        }
        size = 4096;
        @name(".bd_action_profile") implementation = action_profile(32w1024);
        default_action = NoAction();
    }
    apply {
        port_vlan_mapping_0.apply();
    }
}

control process_spanning_tree(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".set_stp_state") action set_stp_state_0(bit<3> stp_state) {
        meta.l2_metadata.stp_state = stp_state;
    }
    @name(".spanning_tree") table spanning_tree_0 {
        actions = {
            set_stp_state_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.ifindex: exact @name("meta.ingress_metadata.ifindex") ;
            meta.l2_metadata.stp_group   : exact @name("meta.l2_metadata.stp_group") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        if (meta.l2_metadata.stp_group != 10w0) 
            spanning_tree_0.apply();
    }
}

control process_ip_sourceguard(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            meta.ingress_metadata.ifindex : exact @name("meta.ingress_metadata.ifindex") ;
            meta.ingress_metadata.bd      : exact @name("meta.ingress_metadata.bd") ;
            meta.l2_metadata.lkp_mac_sa   : exact @name("meta.l2_metadata.lkp_mac_sa") ;
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("meta.ipv4_metadata.lkp_ipv4_sa") ;
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
            meta.l3_metadata.lkp_ip_proto : ternary @name("meta.l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("meta.l3_metadata.lkp_l4_dport") ;
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("meta.ipv4_metadata.lkp_ipv4_da") ;
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

control process_int_endpoint(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control process_ipv4_vtep(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_1() {
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
            nop_1();
            set_tunnel_termination_flag_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf                    : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_da          : exact @name("meta.ipv4_metadata.lkp_ipv4_da") ;
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("meta.tunnel_metadata.ingress_tunnel_type") ;
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
            meta.l3_metadata.vrf          : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("meta.ipv4_metadata.lkp_ipv4_sa") ;
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

control process_ipv6_vtep(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_2() {
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
            nop_2();
            set_tunnel_termination_flag_1();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.vrf                    : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_da          : exact @name("meta.ipv6_metadata.lkp_ipv6_da") ;
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("meta.tunnel_metadata.ingress_tunnel_type") ;
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
            meta.l3_metadata.vrf          : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_sa: exact @name("meta.ipv6_metadata.lkp_ipv6_sa") ;
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

control process_mpls(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            meta.tunnel_metadata.mpls_label: exact @name("meta.tunnel_metadata.mpls_label") ;
            hdr.inner_ipv4.isValid()       : exact @name("hdr.inner_ipv4.isValid()") ;
            hdr.inner_ipv6.isValid()       : exact @name("hdr.inner_ipv6.isValid()") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        mpls_1.apply();
    }
}

control process_tunnel(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".on_miss") action on_miss_3() {
    }
    @name(".outer_rmac_hit") action outer_rmac_hit_0() {
        meta.l3_metadata.rmac_hit = 1w1;
    }
    @name(".nop") action nop_3() {
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
            meta.l3_metadata.rmac_group: exact @name("meta.l3_metadata.rmac_group") ;
            meta.l2_metadata.lkp_mac_da: exact @name("meta.l2_metadata.lkp_mac_da") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".tunnel") table tunnel_0 {
        actions = {
            nop_3();
            terminate_tunnel_inner_non_ip_0();
            terminate_tunnel_inner_ethernet_ipv4_0();
            terminate_tunnel_inner_ipv4_0();
            terminate_tunnel_inner_ethernet_ipv6_0();
            terminate_tunnel_inner_ipv6_0();
            @defaultonly NoAction();
        }
        key = {
            meta.tunnel_metadata.tunnel_vni         : exact @name("meta.tunnel_metadata.tunnel_vni") ;
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("meta.tunnel_metadata.ingress_tunnel_type") ;
            hdr.inner_ipv4.isValid()                : exact @name("hdr.inner_ipv4.isValid()") ;
            hdr.inner_ipv6.isValid()                : exact @name("hdr.inner_ipv6.isValid()") ;
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
                    process_ipv4_vtep_1.apply(hdr, meta, __standard_metadata);
                else 
                    if (hdr.ipv6.isValid()) 
                        process_ipv6_vtep_1.apply(hdr, meta, __standard_metadata);
                    else 
                        if (hdr.mpls[0].isValid()) 
                            process_mpls_1.apply(hdr, meta, __standard_metadata);
            }
        }

        if (meta.tunnel_metadata.tunnel_terminate == 1w1) 
            tunnel_0.apply();
    }
}

control process_validate_packet(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_4() {
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
            nop_4();
            set_unicast_0();
            set_unicast_and_ipv6_src_is_link_local_0();
            set_multicast_0();
            set_multicast_and_ipv6_src_is_link_local_0();
            set_broadcast_0();
            set_malformed_packet_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l2_metadata.lkp_mac_sa[40:40]     : ternary @name("meta.l2_metadata.lkp_mac_sa[40:40]") ;
            meta.l2_metadata.lkp_mac_da            : ternary @name("meta.l2_metadata.lkp_mac_da") ;
            meta.l3_metadata.lkp_ip_type           : ternary @name("meta.l3_metadata.lkp_ip_type") ;
            meta.l3_metadata.lkp_ip_ttl            : ternary @name("meta.l3_metadata.lkp_ip_ttl") ;
            meta.l3_metadata.lkp_ip_version        : ternary @name("meta.l3_metadata.lkp_ip_version") ;
            meta.ipv4_metadata.lkp_ipv4_sa[31:24]  : ternary @name("meta.ipv4_metadata.lkp_ipv4_sa[31:24]") ;
            meta.ipv6_metadata.lkp_ipv6_sa[127:112]: ternary @name("meta.ipv6_metadata.lkp_ipv6_sa[127:112]") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        if (meta.ingress_metadata.drop_flag == 1w0) 
            validate_packet_0.apply();
    }
}

control process_mac(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_5() {
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
            nop_5();
            dmac_hit_0();
            dmac_multicast_hit_0();
            dmac_miss_0();
            dmac_redirect_nexthop_0();
            dmac_redirect_ecmp_0();
            dmac_drop_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.bd   : exact @name("meta.ingress_metadata.bd") ;
            meta.l2_metadata.lkp_mac_da: exact @name("meta.l2_metadata.lkp_mac_da") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name(".smac") table smac_0 {
        actions = {
            nop_5();
            smac_miss_0();
            smac_hit_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.bd   : exact @name("meta.ingress_metadata.bd") ;
            meta.l2_metadata.lkp_mac_sa: exact @name("meta.l2_metadata.lkp_mac_sa") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        smac_0.apply();
        dmac_0.apply();
    }
}

control process_mac_acl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_6() {
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
            nop_6();
            acl_log_0();
            acl_deny_0();
            acl_permit_0();
            acl_mirror_0();
            @defaultonly NoAction();
        }
        key = {
            meta.acl_metadata.if_label   : ternary @name("meta.acl_metadata.if_label") ;
            meta.acl_metadata.bd_label   : ternary @name("meta.acl_metadata.bd_label") ;
            meta.l2_metadata.lkp_mac_sa  : ternary @name("meta.l2_metadata.lkp_mac_sa") ;
            meta.l2_metadata.lkp_mac_da  : ternary @name("meta.l2_metadata.lkp_mac_da") ;
            meta.l2_metadata.lkp_mac_type: ternary @name("meta.l2_metadata.lkp_mac_type") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        mac_acl_0.apply();
    }
}

control process_ip_acl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_7() {
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
            nop_7();
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
            meta.acl_metadata.if_label    : ternary @name("meta.acl_metadata.if_label") ;
            meta.acl_metadata.bd_label    : ternary @name("meta.acl_metadata.bd_label") ;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("meta.ipv4_metadata.lkp_ipv4_sa") ;
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("meta.ipv4_metadata.lkp_ipv4_da") ;
            meta.l3_metadata.lkp_ip_proto : ternary @name("meta.l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_sport : ternary @name("meta.l3_metadata.lkp_l4_sport") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("meta.l3_metadata.lkp_l4_dport") ;
            hdr.tcp.flags                 : ternary @name("hdr.tcp.flags") ;
            meta.l3_metadata.lkp_ip_ttl   : ternary @name("meta.l3_metadata.lkp_ip_ttl") ;
        }
        size = 512;
        default_action = NoAction();
    }
    @name(".ipv6_acl") table ipv6_acl_0 {
        actions = {
            nop_7();
            acl_log_1();
            acl_deny_1();
            acl_permit_1();
            acl_mirror_1();
            acl_redirect_nexthop_0();
            acl_redirect_ecmp_0();
            @defaultonly NoAction();
        }
        key = {
            meta.acl_metadata.if_label    : ternary @name("meta.acl_metadata.if_label") ;
            meta.acl_metadata.bd_label    : ternary @name("meta.acl_metadata.bd_label") ;
            meta.ipv6_metadata.lkp_ipv6_sa: ternary @name("meta.ipv6_metadata.lkp_ipv6_sa") ;
            meta.ipv6_metadata.lkp_ipv6_da: ternary @name("meta.ipv6_metadata.lkp_ipv6_da") ;
            meta.l3_metadata.lkp_ip_proto : ternary @name("meta.l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_sport : ternary @name("meta.l3_metadata.lkp_l4_sport") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("meta.l3_metadata.lkp_l4_dport") ;
            hdr.tcp.flags                 : ternary @name("hdr.tcp.flags") ;
            meta.l3_metadata.lkp_ip_ttl   : ternary @name("meta.l3_metadata.lkp_ip_ttl") ;
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

control process_qos(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_8() {
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
            nop_8();
            apply_cos_marking_0();
            apply_dscp_marking_0();
            apply_tc_marking_0();
            @defaultonly NoAction();
        }
        key = {
            meta.acl_metadata.if_label    : ternary @name("meta.acl_metadata.if_label") ;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("meta.ipv4_metadata.lkp_ipv4_sa") ;
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("meta.ipv4_metadata.lkp_ipv4_da") ;
            meta.l3_metadata.lkp_ip_proto : ternary @name("meta.l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_ip_tc    : ternary @name("meta.l3_metadata.lkp_ip_tc") ;
            meta.tunnel_metadata.mpls_exp : ternary @name("meta.tunnel_metadata.mpls_exp") ;
            meta.qos_metadata.outer_dscp  : ternary @name("meta.qos_metadata.outer_dscp") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        qos_0.apply();
    }
}

control process_ipv4_racl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_9() {
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
            nop_9();
            racl_log_0();
            racl_deny_0();
            racl_permit_0();
            racl_redirect_nexthop_0();
            racl_redirect_ecmp_0();
            @defaultonly NoAction();
        }
        key = {
            meta.acl_metadata.bd_label    : ternary @name("meta.acl_metadata.bd_label") ;
            meta.ipv4_metadata.lkp_ipv4_sa: ternary @name("meta.ipv4_metadata.lkp_ipv4_sa") ;
            meta.ipv4_metadata.lkp_ipv4_da: ternary @name("meta.ipv4_metadata.lkp_ipv4_da") ;
            meta.l3_metadata.lkp_ip_proto : ternary @name("meta.l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_sport : ternary @name("meta.l3_metadata.lkp_l4_sport") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("meta.l3_metadata.lkp_l4_dport") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        ipv4_racl_0.apply();
    }
}

control process_ipv4_urpf(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            meta.l3_metadata.vrf          : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_sa: exact @name("meta.ipv4_metadata.lkp_ipv4_sa") ;
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
            meta.l3_metadata.vrf          : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_sa: lpm @name("meta.ipv4_metadata.lkp_ipv4_sa") ;
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

control process_ipv4_fib(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            meta.l3_metadata.vrf          : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_da: exact @name("meta.ipv4_metadata.lkp_ipv4_da") ;
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
            meta.l3_metadata.vrf          : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv4_metadata.lkp_ipv4_da: lpm @name("meta.ipv4_metadata.lkp_ipv4_da") ;
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

control process_ipv6_racl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_10() {
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
            nop_10();
            racl_log_1();
            racl_deny_1();
            racl_permit_1();
            racl_redirect_nexthop_1();
            racl_redirect_ecmp_1();
            @defaultonly NoAction();
        }
        key = {
            meta.acl_metadata.bd_label    : ternary @name("meta.acl_metadata.bd_label") ;
            meta.ipv6_metadata.lkp_ipv6_sa: ternary @name("meta.ipv6_metadata.lkp_ipv6_sa") ;
            meta.ipv6_metadata.lkp_ipv6_da: ternary @name("meta.ipv6_metadata.lkp_ipv6_da") ;
            meta.l3_metadata.lkp_ip_proto : ternary @name("meta.l3_metadata.lkp_ip_proto") ;
            meta.l3_metadata.lkp_l4_sport : ternary @name("meta.l3_metadata.lkp_l4_sport") ;
            meta.l3_metadata.lkp_l4_dport : ternary @name("meta.l3_metadata.lkp_l4_dport") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        ipv6_racl_0.apply();
    }
}

control process_ipv6_urpf(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            meta.l3_metadata.vrf          : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_sa: exact @name("meta.ipv6_metadata.lkp_ipv6_sa") ;
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
            meta.l3_metadata.vrf          : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_sa: lpm @name("meta.ipv6_metadata.lkp_ipv6_sa") ;
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

control process_ipv6_fib(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            meta.l3_metadata.vrf          : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_da: exact @name("meta.ipv6_metadata.lkp_ipv6_da") ;
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
            meta.l3_metadata.vrf          : exact @name("meta.l3_metadata.vrf") ;
            meta.ipv6_metadata.lkp_ipv6_da: lpm @name("meta.ipv6_metadata.lkp_ipv6_da") ;
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

control process_urpf_bd(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_11() {
    }
    @name(".urpf_bd_miss") action urpf_bd_miss_0() {
        meta.l3_metadata.urpf_check_fail = 1w1;
    }
    @name(".urpf_bd") table urpf_bd_0 {
        actions = {
            nop_11();
            urpf_bd_miss_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.urpf_bd_group: exact @name("meta.l3_metadata.urpf_bd_group") ;
            meta.ingress_metadata.bd      : exact @name("meta.ingress_metadata.bd") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        if (meta.l3_metadata.urpf_mode == 2w2 && meta.l3_metadata.urpf_hit == 1w1) 
            urpf_bd_0.apply();
    }
}

control process_ingress_fabric(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_12() {
    }
    @name(".terminate_cpu_packet") action terminate_cpu_packet_0() {
        meta.standard_metadata.egress_spec = (bit<9>)hdr.fabric_header.dstPortOrGroup;
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
        meta.standard_metadata.egress_spec = (bit<9>)hdr.fabric_header.dstPortOrGroup;
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
            nop_12();
            terminate_cpu_packet_0();
            switch_fabric_unicast_packet_0();
            terminate_fabric_unicast_packet_0();
            switch_fabric_multicast_packet_0();
            terminate_fabric_multicast_packet_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.fabric_header.dstDevice: exact @name("hdr.fabric_header.dstDevice") ;
        }
        default_action = NoAction();
    }
    @name(".fabric_ingress_src_lkp") table fabric_ingress_src_lkp_0 {
        actions = {
            nop_12();
            set_ingress_ifindex_properties_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.fabric_header_multicast.ingressIfindex: exact @name("hdr.fabric_header_multicast.ingressIfindex") ;
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
            meta.tunnel_metadata.ingress_tunnel_type: exact @name("meta.tunnel_metadata.ingress_tunnel_type") ;
            hdr.inner_ipv4.isValid()                : exact @name("hdr.inner_ipv4.isValid()") ;
            hdr.inner_ipv6.isValid()                : exact @name("hdr.inner_ipv6.isValid()") ;
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

control process_hashes(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    bit<16> tmp_0;
    tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>> tmp_1;
    bit<16> tmp_2;
    tuple<bit<48>, bit<48>, bit<32>, bit<32>, bit<8>, bit<16>, bit<16>> tmp_3;
    bit<16> tmp_4;
    tuple<bit<128>, bit<128>, bit<8>, bit<16>, bit<16>> tmp_5;
    bit<16> tmp_6;
    tuple<bit<48>, bit<48>, bit<128>, bit<128>, bit<8>, bit<16>, bit<16>> tmp_7;
    bit<16> tmp_8;
    tuple<bit<16>, bit<48>, bit<48>, bit<16>> tmp_9;
    @name(".compute_lkp_ipv4_hash") action compute_lkp_ipv4_hash_0() {
        tmp_1 = { meta.ipv4_metadata.lkp_ipv4_sa, meta.ipv4_metadata.lkp_ipv4_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport };
        hash<bit<16>, bit<16>, tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>>, bit<32>>(tmp_0, HashAlgorithm.crc16, 16w0, tmp_1, 32w65536);
        meta.hash_metadata.hash1 = tmp_0;
        tmp_3 = { meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.ipv4_metadata.lkp_ipv4_sa, meta.ipv4_metadata.lkp_ipv4_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport };
        hash<bit<16>, bit<16>, tuple<bit<48>, bit<48>, bit<32>, bit<32>, bit<8>, bit<16>, bit<16>>, bit<32>>(tmp_2, HashAlgorithm.crc16, 16w0, tmp_3, 32w65536);
        meta.hash_metadata.hash2 = tmp_2;
    }
    @name(".compute_lkp_ipv6_hash") action compute_lkp_ipv6_hash_0() {
        tmp_5 = { meta.ipv6_metadata.lkp_ipv6_sa, meta.ipv6_metadata.lkp_ipv6_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport };
        hash<bit<16>, bit<16>, tuple<bit<128>, bit<128>, bit<8>, bit<16>, bit<16>>, bit<32>>(tmp_4, HashAlgorithm.crc16, 16w0, tmp_5, 32w65536);
        meta.hash_metadata.hash1 = tmp_4;
        tmp_7 = { meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.ipv6_metadata.lkp_ipv6_sa, meta.ipv6_metadata.lkp_ipv6_da, meta.l3_metadata.lkp_ip_proto, meta.l3_metadata.lkp_l4_sport, meta.l3_metadata.lkp_l4_dport };
        hash<bit<16>, bit<16>, tuple<bit<48>, bit<48>, bit<128>, bit<128>, bit<8>, bit<16>, bit<16>>, bit<32>>(tmp_6, HashAlgorithm.crc16, 16w0, tmp_7, 32w65536);
        meta.hash_metadata.hash2 = tmp_6;
    }
    @name(".compute_lkp_non_ip_hash") action compute_lkp_non_ip_hash_0() {
        tmp_9 = { meta.ingress_metadata.ifindex, meta.l2_metadata.lkp_mac_sa, meta.l2_metadata.lkp_mac_da, meta.l2_metadata.lkp_mac_type };
        hash<bit<16>, bit<16>, tuple<bit<16>, bit<48>, bit<48>, bit<16>>, bit<32>>(tmp_8, HashAlgorithm.crc16, 16w0, tmp_9, 32w65536);
        meta.hash_metadata.hash2 = tmp_8;
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
            meta.ingress_metadata.drop_flag: exact @name("meta.ingress_metadata.drop_flag") ;
        }
        default_action = NoAction();
    }
    @name(".compute_ipv6_hashes") table compute_ipv6_hashes_0 {
        actions = {
            compute_lkp_ipv6_hash_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("meta.ingress_metadata.drop_flag") ;
        }
        default_action = NoAction();
    }
    @name(".compute_non_ip_hashes") table compute_non_ip_hashes_0 {
        actions = {
            compute_lkp_non_ip_hash_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.drop_flag: exact @name("meta.ingress_metadata.drop_flag") ;
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
            meta.hash_metadata.hash1: exact @name("meta.hash_metadata.hash1") ;
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

control process_ingress_bd_stats(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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

control process_ingress_acl_stats(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            meta.acl_metadata.acl_stats_index: exact @name("meta.acl_metadata.acl_stats_index") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        acl_stats_0.apply();
    }
}

control process_fwd_results(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_13() {
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
        meta.standard_metadata.egress_spec = 9w64;
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
            nop_13();
            set_l2_redirect_action_0();
            set_fib_redirect_action_0();
            set_cpu_redirect_action_0();
            set_acl_redirect_action_0();
            set_racl_redirect_action_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l2_metadata.l2_redirect   : ternary @name("meta.l2_metadata.l2_redirect") ;
            meta.acl_metadata.acl_redirect : ternary @name("meta.acl_metadata.acl_redirect") ;
            meta.acl_metadata.racl_redirect: ternary @name("meta.acl_metadata.racl_redirect") ;
            meta.l3_metadata.rmac_hit      : ternary @name("meta.l3_metadata.rmac_hit") ;
            meta.l3_metadata.fib_hit       : ternary @name("meta.l3_metadata.fib_hit") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        fwd_result_0.apply();
    }
}

control process_nexthop(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_14() {
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
            nop_14();
            set_ecmp_nexthop_details_0();
            set_ecmp_nexthop_details_for_post_routed_flood_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("meta.l3_metadata.nexthop_index") ;
            meta.hash_metadata.hash1      : selector @name("meta.hash_metadata.hash1") ;
        }
        size = 1024;
        @name(".ecmp_action_profile") @mode("fair") implementation = action_selector(HashAlgorithm.identity, 32w1024, 32w10);
        default_action = NoAction();
    }
    @name(".nexthop") table nexthop_0 {
        actions = {
            nop_14();
            set_nexthop_details_0();
            set_nexthop_details_for_post_routed_flood_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l3_metadata.nexthop_index: exact @name("meta.l3_metadata.nexthop_index") ;
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

control process_multicast_flooding(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_15() {
    }
    @name(".set_bd_flood_mc_index") action set_bd_flood_mc_index_0(bit<16> mc_index) {
        meta.intrinsic_metadata.mcast_grp = mc_index;
    }
    @name(".bd_flood") table bd_flood_0 {
        actions = {
            nop_15();
            set_bd_flood_mc_index_0();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.bd     : exact @name("meta.ingress_metadata.bd") ;
            meta.l2_metadata.lkp_pkt_type: exact @name("meta.l2_metadata.lkp_pkt_type") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        bd_flood_0.apply();
    }
}

control process_lag(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".set_lag_miss") action set_lag_miss_0() {
    }
    @name(".set_lag_port") action set_lag_port_0(bit<9> port) {
        meta.standard_metadata.egress_spec = port;
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
            meta.ingress_metadata.egress_ifindex: exact @name("meta.ingress_metadata.egress_ifindex") ;
            meta.hash_metadata.hash2            : selector @name("meta.hash_metadata.hash2") ;
        }
        size = 1024;
        @name(".lag_action_profile") @mode("fair") implementation = action_selector(HashAlgorithm.identity, 32w1024, 32w8);
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

control process_mac_learning(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_16() {
    }
    @name(".generate_learn_notify") action generate_learn_notify_0() {
        digest<mac_learn_digest>(32w1024, { meta.ingress_metadata.bd, meta.l2_metadata.lkp_mac_sa, meta.ingress_metadata.ifindex });
    }
    @name(".learn_notify") table learn_notify_0 {
        actions = {
            nop_16();
            generate_learn_notify_0();
            @defaultonly NoAction();
        }
        key = {
            meta.l2_metadata.l2_src_miss: ternary @name("meta.l2_metadata.l2_src_miss") ;
            meta.l2_metadata.l2_src_move: ternary @name("meta.l2_metadata.l2_src_move") ;
            meta.l2_metadata.stp_state  : ternary @name("meta.l2_metadata.stp_state") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        if (meta.l2_metadata.learning_enabled == 1w1) 
            learn_notify_0.apply();
    }
}

control process_fabric_lag(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_17() {
    }
    @name(".set_fabric_lag_port") action set_fabric_lag_port_0(bit<9> port) {
        meta.standard_metadata.egress_spec = port;
    }
    @name(".set_fabric_multicast") action set_fabric_multicast_0(bit<8> fabric_mgid) {
        meta.multicast_metadata.mcast_grp = meta.intrinsic_metadata.mcast_grp;
    }
    @name(".fabric_lag") table fabric_lag_0 {
        actions = {
            nop_17();
            set_fabric_lag_port_0();
            set_fabric_multicast_0();
            @defaultonly NoAction();
        }
        key = {
            meta.fabric_metadata.dst_device: exact @name("meta.fabric_metadata.dst_device") ;
            meta.hash_metadata.hash2       : selector @name("meta.hash_metadata.hash2") ;
        }
        @name(".fabric_lag_action_profile") @mode("fair") implementation = action_selector(HashAlgorithm.identity, 32w1024, 32w8);
        default_action = NoAction();
    }
    apply {
        fabric_lag_0.apply();
    }
}

control process_system_acl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".drop_stats") counter(32w1024, CounterType.packets) drop_stats_1;
    @name(".drop_stats_2") counter(32w1024, CounterType.packets) drop_stats_3;
    @name(".drop_stats_update") action drop_stats_update_0() {
        drop_stats_3.count((bit<32>)meta.ingress_metadata.drop_reason);
    }
    @name(".nop") action nop_18() {
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
            nop_18();
            redirect_to_cpu_0();
            copy_to_cpu_0();
            drop_packet_0();
            drop_packet_with_reason_0();
            negative_mirror_0();
            congestion_mirror_set_0();
            @defaultonly NoAction();
        }
        key = {
            meta.acl_metadata.if_label               : ternary @name("meta.acl_metadata.if_label") ;
            meta.acl_metadata.bd_label               : ternary @name("meta.acl_metadata.bd_label") ;
            meta.ipv4_metadata.lkp_ipv4_sa           : ternary @name("meta.ipv4_metadata.lkp_ipv4_sa") ;
            meta.ipv4_metadata.lkp_ipv4_da           : ternary @name("meta.ipv4_metadata.lkp_ipv4_da") ;
            meta.l3_metadata.lkp_ip_proto            : ternary @name("meta.l3_metadata.lkp_ip_proto") ;
            meta.l2_metadata.lkp_mac_sa              : ternary @name("meta.l2_metadata.lkp_mac_sa") ;
            meta.l2_metadata.lkp_mac_da              : ternary @name("meta.l2_metadata.lkp_mac_da") ;
            meta.l2_metadata.lkp_mac_type            : ternary @name("meta.l2_metadata.lkp_mac_type") ;
            meta.ingress_metadata.ifindex            : ternary @name("meta.ingress_metadata.ifindex") ;
            meta.l2_metadata.port_vlan_mapping_miss  : ternary @name("meta.l2_metadata.port_vlan_mapping_miss") ;
            meta.security_metadata.ipsg_check_fail   : ternary @name("meta.security_metadata.ipsg_check_fail") ;
            meta.acl_metadata.acl_deny               : ternary @name("meta.acl_metadata.acl_deny") ;
            meta.acl_metadata.racl_deny              : ternary @name("meta.acl_metadata.racl_deny") ;
            meta.l3_metadata.urpf_check_fail         : ternary @name("meta.l3_metadata.urpf_check_fail") ;
            meta.ingress_metadata.drop_flag          : ternary @name("meta.ingress_metadata.drop_flag") ;
            meta.l3_metadata.rmac_hit                : ternary @name("meta.l3_metadata.rmac_hit") ;
            meta.l3_metadata.routed                  : ternary @name("meta.l3_metadata.routed") ;
            meta.ipv6_metadata.ipv6_src_is_link_local: ternary @name("meta.ipv6_metadata.ipv6_src_is_link_local") ;
            meta.l2_metadata.same_if_check           : ternary @name("meta.l2_metadata.same_if_check") ;
            meta.tunnel_metadata.tunnel_if_check     : ternary @name("meta.tunnel_metadata.tunnel_if_check") ;
            meta.l3_metadata.same_bd_check           : ternary @name("meta.l3_metadata.same_bd_check") ;
            meta.l3_metadata.lkp_ip_ttl              : ternary @name("meta.l3_metadata.lkp_ip_ttl") ;
            meta.l2_metadata.stp_state               : ternary @name("meta.l2_metadata.stp_state") ;
            meta.ingress_metadata.control_frame      : ternary @name("meta.ingress_metadata.control_frame") ;
            meta.ipv4_metadata.ipv4_unicast_enabled  : ternary @name("meta.ipv4_metadata.ipv4_unicast_enabled") ;
            meta.ingress_metadata.egress_ifindex     : ternary @name("meta.ingress_metadata.egress_ifindex") ;
            meta.ingress_metadata.enable_dod         : ternary @name("meta.ingress_metadata.enable_dod") ;
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

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
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
            meta.l3_metadata.rmac_group: exact @name("meta.l3_metadata.rmac_group") ;
            meta.l2_metadata.lkp_mac_da: exact @name("meta.l2_metadata.lkp_mac_da") ;
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
        process_ingress_port_mapping_1.apply(hdr, meta, __standard_metadata);
        process_validate_outer_header_1.apply(hdr, meta, __standard_metadata);
        if (meta.ingress_metadata.port_type == 2w0) {
            process_storm_control_1.apply(hdr, meta, __standard_metadata);
            process_port_vlan_mapping_1.apply(hdr, meta, __standard_metadata);
            process_spanning_tree_1.apply(hdr, meta, __standard_metadata);
            process_ip_sourceguard_1.apply(hdr, meta, __standard_metadata);
            process_int_endpoint_1.apply(hdr, meta, __standard_metadata);
            process_tunnel_1.apply(hdr, meta, __standard_metadata);
            if (!hdr.mpls[0].isValid() || hdr.mpls[0].isValid() && meta.tunnel_metadata.tunnel_terminate == 1w1) {
                process_validate_packet_1.apply(hdr, meta, __standard_metadata);
                process_mac_1.apply(hdr, meta, __standard_metadata);
                if (meta.l3_metadata.lkp_ip_type == 2w0) 
                    process_mac_acl_1.apply(hdr, meta, __standard_metadata);
                else 
                    process_ip_acl_1.apply(hdr, meta, __standard_metadata);
                process_qos_1.apply(hdr, meta, __standard_metadata);
                switch (rmac_0.apply().action_run) {
                    rmac_hit_0: {
                        if (meta.l3_metadata.lkp_ip_type == 2w1 && meta.ipv4_metadata.ipv4_unicast_enabled == 1w1) {
                            process_ipv4_racl_1.apply(hdr, meta, __standard_metadata);
                            process_ipv4_urpf_1.apply(hdr, meta, __standard_metadata);
                            process_ipv4_fib_1.apply(hdr, meta, __standard_metadata);
                        }
                        else 
                            if (meta.l3_metadata.lkp_ip_type == 2w2 && meta.ipv6_metadata.ipv6_unicast_enabled == 1w1) {
                                process_ipv6_racl_1.apply(hdr, meta, __standard_metadata);
                                process_ipv6_urpf_1.apply(hdr, meta, __standard_metadata);
                                process_ipv6_fib_1.apply(hdr, meta, __standard_metadata);
                            }
                        process_urpf_bd_1.apply(hdr, meta, __standard_metadata);
                    }
                }

            }
        }
        else 
            process_ingress_fabric_1.apply(hdr, meta, __standard_metadata);
        process_hashes_1.apply(hdr, meta, __standard_metadata);
        if (meta.ingress_metadata.port_type == 2w0) {
            process_ingress_bd_stats_1.apply(hdr, meta, __standard_metadata);
            process_ingress_acl_stats_1.apply(hdr, meta, __standard_metadata);
            process_fwd_results_1.apply(hdr, meta, __standard_metadata);
            process_nexthop_1.apply(hdr, meta, __standard_metadata);
            if (meta.ingress_metadata.egress_ifindex == 16w65535) 
                process_multicast_flooding_1.apply(hdr, meta, __standard_metadata);
            else 
                process_lag_1.apply(hdr, meta, __standard_metadata);
            process_mac_learning_1.apply(hdr, meta, __standard_metadata);
        }
        process_fabric_lag_1.apply(hdr, meta, __standard_metadata);
        process_system_acl_1.apply(hdr, meta, __standard_metadata);
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
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

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    bool tmp_10;
    bit<16> tmp_11;
    bool tmp_12;
    bool tmp_13;
    bit<16> tmp_14;
    bool tmp_15;
    @name("inner_ipv4_checksum") Checksum16() inner_ipv4_checksum_0;
    @name("ipv4_checksum") Checksum16() ipv4_checksum_0;
    apply {
        if (!(hdr.inner_ipv4.ihl == 4w5)) 
            tmp_10 = false;
        else {
            tmp_11 = inner_ipv4_checksum_0.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr });
            tmp_12 = hdr.inner_ipv4.hdrChecksum == tmp_11;
            tmp_10 = tmp_12;
        }
        if (tmp_10) 
            mark_to_drop();
        if (!(hdr.ipv4.ihl == 4w5)) 
            tmp_13 = false;
        else {
            tmp_14 = ipv4_checksum_0.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
            tmp_15 = hdr.ipv4.hdrChecksum == tmp_14;
            tmp_13 = tmp_15;
        }
        if (tmp_13) 
            mark_to_drop();
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    bit<16> tmp_16;
    bit<16> tmp_17;
    @name("inner_ipv4_checksum") Checksum16() inner_ipv4_checksum_1;
    @name("ipv4_checksum") Checksum16() ipv4_checksum_1;
    apply {
        if (hdr.inner_ipv4.ihl == 4w5) {
            tmp_16 = inner_ipv4_checksum_1.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr });
            hdr.inner_ipv4.hdrChecksum = tmp_16;
        }
        if (hdr.ipv4.ihl == 4w5) {
            tmp_17 = ipv4_checksum_1.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
            hdr.ipv4.hdrChecksum = tmp_17;
        }
    }
}

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
