// Copyright (c) 2016, Google Inc.
//
// P4 specification for a ToR (top-of-rack) switch.
// Status: WORK IN PROGRESS
// Note: This code has not been tested and is expected to contain bugs.

//------------------------------------------------------------------------------
// Global defines
//------------------------------------------------------------------------------

#define CPU_PORT 64
#define INITIAL_CPU_PACKET_OFFSET 64

#define ARP_REASON 1

#define ETHERTYPE_VLAN 0x8100, 0x9100, 0x9200, 0x9300
#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_IPV6 0x86dd
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_ND 0x6007
#define ETHERTYPE_LLDP 0x88CC

#define IP_PROTOCOLS_TCP 6
#define IP_PROTOCOLS_UDP 17
#define IP_PROTOCOLS_ICMP 1
#define IP_PROTOCOLS_ICMPv6 58

#define DEFAULT_VRF0 0

#define L2_VLAN 4050
#define VLAN_DEPTH 2

#define CPU_MIRROR_SESSION_ID 1024

#define PORT_COUNT 256

#ifdef P4_EXPLICIT_LAG 
#include "lag.p4"
#endif

//------------------------------------------------------------------------------
// Protocol Header Definitions
//------------------------------------------------------------------------------

header_type ethernet_t {
  fields {
    dstAddr : 48;
    srcAddr : 48;
    etherType : 16;
  }
}

header_type ipv4_base_t {
  fields {
    version : 4;
    ihl : 4;
    diffserv : 8;
    totalLen : 16;
    identification : 16;
    flags : 3;
    fragOffset : 13;
    ttl : 8;
    protocol : 8;
    hdrChecksum : 16;
    srcAddr : 32;
    dstAddr : 32;
  }
}

// Fixed ipv6 header
header_type ipv6_base_t {
  fields {
    version : 4;
    traffic_class : 8;
    flowLabel : 20;
    payloadLength : 16;
    nextHeader : 8;
    hopLimit : 8;
    srcAddr : 128;
    dstAddr : 128;
  }
}

header_type udp_t {
  fields {
    srcPort : 16;
    dstPort : 16;
    hdr_length : 16;
    checksum : 16;
  }
}

header_type tcp_t {
  fields {
    srcPort : 16;
    dstPort : 16;
    seqNo : 32;
    ackNo : 32;
    dataOffset : 4;
    res : 4;
    flags : 8;
    window : 16;
    checksum : 16;
    urgentPtr : 16;
  }
}

// Same for both ip v4 and v6
header_type icmp_header_t {
  fields {
    icmpType: 8;
    code: 8;
    checksum: 16;
  }
}

header_type vlan_tag_t {
  fields {
    pcp : 3;
    cfi : 1;
    vid : 12;
    etherType : 16;
  }
}

header_type arp_t {
  fields {
    hwType : 16;
    protoType : 16;
    hwAddrLen : 8;
    protoAddrLen : 8;
    opcode : 16;
    hwSrcAddr : 48;
    protoSrcAddr : 32;
    hwDstAddr : 48;
    protoDstAddr : 32;
  }
}

//------------------------------------------------------------------------------
// Internal Header Definitions
//------------------------------------------------------------------------------

header_type cpu_header_t {
  fields {
    zeros : 64;
    reason : 16;
    port : 32;
  }
}

//------------------------------------------------------------------------------
// Metadata Header and Field List Definitions
//------------------------------------------------------------------------------

// Local meta-data for each packet being processed.
header_type local_metadata_t {
  fields {
    vrf_id : 32;
    class_id: 8;  // Dst traffic class ID (IPSP)
    qid: 5;  // CPU COS queue ID
    ingress_meter_index: 16;  // per-port ingress rate-limiting (IPSP)
    color: 2;
    l4SrcPort: 16;
    l4DstPort: 16;
    icmp_code: 8;
    reason: 8; // Reason to drop to CPU
    src_mac: 48;
  }
}

// Field List for packets destined to CPU.
field_list cpu_info {
  local_metadata.reason;
  standard_metadata.ingress_port;
}

//------------------------------------------------------------------------------
// Headers and Metadata Declarations
//------------------------------------------------------------------------------

header ethernet_t ethernet;
header ipv4_base_t ipv4_base;
header ipv6_base_t ipv6_base;
header icmp_header_t icmp_header;
header tcp_t tcp;
header udp_t udp;
header vlan_tag_t vlan_tag[VLAN_DEPTH];
header arp_t arp;

header cpu_header_t cpu_header;

metadata local_metadata_t local_metadata;

//------------------------------------------------------------------------------
// Parsers
//------------------------------------------------------------------------------

// Start with ethernet always.
parser start {
  return select(current(0, INITIAL_CPU_PACKET_OFFSET)) {
    0 :      parse_cpu_header;
    default: parse_ethernet;
  }
}

parser parse_ethernet {
  extract(ethernet);
  return select(latest.etherType) {
    ETHERTYPE_VLAN: parse_vlan;
    ETHERTYPE_IPV4: parse_ipv4;
    ETHERTYPE_IPV6: parse_ipv6;
    ETHERTYPE_ARP:  parse_arp;
    default: ingress;
  }
}

parser parse_vlan {
  extract(vlan_tag[next]);
  return select(latest.etherType) {
    ETHERTYPE_VLAN : parse_vlan;
    ETHERTYPE_IPV4 : parse_ipv4;
    ETHERTYPE_IPV6 : parse_ipv6;
    default: ingress;
  }
}

parser parse_ipv4 {
  extract(ipv4_base);
  return select(latest.fragOffset, latest.protocol) {
    IP_PROTOCOLS_ICMP : parse_icmp;
    IP_PROTOCOLS_TCP  : parse_tcp;
    IP_PROTOCOLS_UDP  : parse_udp;
    default: ingress;
  }
}

parser parse_ipv6 {
  extract(ipv6_base);
  return select(ipv6_base.nextHeader) {
    IP_PROTOCOLS_ICMPv6: parse_icmp;
    IP_PROTOCOLS_TCP   : parse_tcp;
    IP_PROTOCOLS_UDP   : parse_udp;
    default: ingress;
  }
}

parser parse_tcp {
  extract(tcp);
  // Normalize TCP port metadata to common port metadata
  set_metadata(local_metadata.l4SrcPort, latest.srcPort);
  set_metadata(local_metadata.l4DstPort, latest.dstPort);
  return ingress;
}

parser parse_udp {
  extract(udp);
  // Normalize UDP port metadata to common port metadata
  set_metadata(local_metadata.l4SrcPort, latest.srcPort);
  set_metadata(local_metadata.l4DstPort, latest.dstPort);
  return ingress;
}

parser parse_icmp {
  extract(icmp_header);
  return ingress;
}

parser parse_arp {
  extract(arp);
  return ingress;
}

parser parse_cpu_header {
  extract(cpu_header);
  return parse_ethernet;
}

//------------------------------------------------------------------------------
// Actions
//------------------------------------------------------------------------------

// Do nothing action.
action nop() {
}

// Drops the packet.
action drop_packet() {
  drop();
}

// TODO(wmohsin): Confirm this use-case.
// Sets the ’special' L2 Vlan that we allow (every other L2 packet forwarding is
// disabled).
action set_l2_vlan() {
  modify_field(vlan_tag[0].vid, L2_VLAN);
  modify_field(vlan_tag[0].etherType, 0x8100);
}

action set_class_id(class_id) {
  modify_field(local_metadata.class_id, class_id);
}

action set_vrf(vrf_id) {
  modify_field(local_metadata.vrf_id, vrf_id);
}

action set_ecmp_nexthop_info_port(port, smac, dmac) {
  modify_field(standard_metadata.egress_spec, port);
  modify_field(ethernet.srcAddr, smac);
  modify_field(ethernet.dstAddr, dmac);
}

//-----------------------------------
// Send/Copy to CPU in a given queue
//-----------------------------------

action set_queue_and_copy_to_cpu(qid, reason) {
  modify_field(local_metadata.qid, qid);
  modify_field(local_metadata.reason, reason);
  clone_ingress_pkt_to_egress(CPU_MIRROR_SESSION_ID, cpu_info);
}

action set_queue_and_send_to_cpu(qid, reason) {
  modify_field(local_metadata.qid, qid);
  add_header(cpu_header);
  modify_field(cpu_header.reason, reason);
  modify_field(cpu_header.port, standard_metadata.ingress_port);
  modify_field(standard_metadata.egress_spec, CPU_PORT);
}

//------------------------------------------------------------------------------
// Receive from CPU 
//------------------------------------------------------------------------------

action set_egress_port_and_decap_cpu_header() {
  modify_field(standard_metadata.egress_spec, cpu_header.port);
  remove_header(cpu_header);
}

action meter_packet(meter_index) {
  modify_field(local_metadata.ingress_meter_index, meter_index);
  execute_meter(ingress_port_meter, meter_index, local_metadata.color);
}

action meter_deny() {
  drop();
}

action meter_permit() {
}

//------------------------------------------------------------------------------
// Tables and Control Flows
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Packet Classification
//------------------------------------------------------------------------------

table class_id_assignment_table {
  reads {
    ethernet.etherType: exact;

    ipv4_base.ttl: ternary;
    ipv6_base.hopLimit: ternary;
    ipv4_base.dstAddr: ternary;
    ipv6_base.dstAddr: ternary;
    ipv4_base.protocol: exact;
    ipv6_base.nextHeader: exact;

    local_metadata.l4SrcPort: exact;
    local_metadata.l4DstPort: exact;

    vlan_tag[0].vid: exact;
    vlan_tag[0].pcp: exact;
  }
  actions {
    set_class_id;
  }
  default_action: set_class_id(0);
}

//-----------------------------------
// Map traffic to a particular VRF
//-----------------------------------

table vrf_classifier_table {
  reads {
    ethernet.etherType : exact;
    ethernet.srcAddr : exact;
    ethernet.dstAddr : exact;
    standard_metadata.ingress_port: exact;
  }
  actions {
    set_vrf;
  }
  default_action: set_vrf(DEFAULT_VRF0);
}

//------------------------------------------------------------------------------
// L3 Routing (My MAC check)
//------------------------------------------------------------------------------

// Drops packets that do not need to be part of L3 forwarding.
// Equivalent of BCM MY_STATION table ?
table l3_routing_classifier_table {
  reads {
    ethernet.dstAddr : exact;
  }
  actions {
    nop;
    drop_packet;
  }
  default_action: drop_packet();
}

//------------------------------------------------------------------------------
// IPv4 and IPv6 L3 Forwarding
//------------------------------------------------------------------------------

//-----------------------------------
// IPv4 L3 Forwarding
//-----------------------------------

table l3_ipv4_override_table {
  reads {
    ipv4_base.dstAddr : lpm;
  }
  action_profile: ecmp_action_profile;
}

table l3_ipv4_vrf_table {
  reads {
    local_metadata.vrf_id: exact;
    ipv4_base.dstAddr : lpm;
  }
  action_profile: ecmp_action_profile;
}

table l3_ipv4_fallback_table {
  reads {
    ipv4_base.dstAddr : lpm;
  }
  action_profile: ecmp_action_profile;
}

// LPM forwarding for IPV4 packets.
control ingress_ipv4_l3_forwarding {
  apply(l3_ipv4_override_table) {
    miss {
      apply(l3_ipv4_vrf_table) {
        miss {
          apply(l3_ipv4_fallback_table);
        }
      }
    }
  }
}

//-----------------------------------
// IPv6 L3 Forwarding
//-----------------------------------

table l3_ipv6_override_table {
  reads {
    ipv6_base.dstAddr : lpm;
  }
  action_profile: ecmp_action_profile;
}

table l3_ipv6_vrf_table {
  reads {
    local_metadata.vrf_id: exact;
    ipv6_base.dstAddr : lpm;
  }
  action_profile: ecmp_action_profile;
}

table l3_ipv6_fallback_table {
  reads {
    ipv6_base.dstAddr : lpm;
  }
  action_profile: ecmp_action_profile;
}


//-----------------------------------
// L3 ECMP nexthop selection
//-----------------------------------

field_list l3_ip_hash_fields {
  ipv6_base.dstAddr;
  ipv6_base.srcAddr;
  ipv6_base.flowLabel;
  ipv4_base.dstAddr;
  ipv4_base.srcAddr;
  ipv4_base.protocol;
  local_metadata.l4SrcPort;
  local_metadata.l4DstPort;
}

field_list_calculation ecmp_hash {
    input {
        l3_ip_hash_fields;
    }
    algorithm : crc16;
    output_width : 14;
}

action_selector ecmp_selector {
    selection_key : ecmp_hash;
    selection_mode : fair;
}

action_profile ecmp_action_profile {
  actions {
    set_ecmp_nexthop_info_port;
  }
  dynamic_action_selection: ecmp_selector;
}

// LPM forwarding for IPV6 packets.
control ingress_ipv6_l3_forwarding {
  apply(l3_ipv6_override_table) {
    miss {
      apply(l3_ipv6_vrf_table) {
        miss {
          apply(l3_ipv6_fallback_table);
        }
      }
    }
  }
}

//-----------------------------------
// IPv4/v6 L3 Forwarding
//-----------------------------------

// Controls LPM forwarding.
control ingress_lpm_forwarding {
  if (valid(ipv4_base)) {
    ingress_ipv4_l3_forwarding();
  } else {
    if (valid(ipv6_base)) {
      ingress_ipv6_l3_forwarding();
    }
  }
#ifdef P4_EXPLICIT_LAG
  lag_handling();
#endif
}

// Combined punt table.
// TODO(wmohsin): target_egress_port in punted packet-io.
table punt_table {
  reads {
    standard_metadata.ingress_port: ternary;
    standard_metadata.egress_spec: ternary;

    ethernet.etherType: ternary;

    ipv4_base: valid;
    ipv6_base: valid;
    ipv4_base.diffserv: ternary;
    ipv6_base.traffic_class: ternary;
    ipv4_base.ttl: ternary;
    ipv6_base.hopLimit: ternary;
    ipv4_base.srcAddr: ternary;
    ipv4_base.dstAddr: ternary;
    ipv6_base.srcAddr: ternary;
    ipv6_base.dstAddr: ternary;
    ipv4_base.protocol: ternary;
    ipv6_base.nextHeader: ternary;

    arp.protoDstAddr: ternary;
    local_metadata.icmp_code: ternary;

    vlan_tag[0].vid: ternary;
    vlan_tag[0].pcp: ternary;

    local_metadata.class_id: ternary;
    local_metadata.vrf_id: ternary;
  }
  actions {
    set_queue_and_copy_to_cpu;
    set_queue_and_send_to_cpu;
  }
}

table process_cpu_header {
  actions {
    set_egress_port_and_decap_cpu_header;
  }
  default_action: set_egress_port_and_decap_cpu_header();
}

//------------------------------------------------------------------------------
// Meters
//------------------------------------------------------------------------------

// TODO(wmohsin): Evaluate this being direct: ingress_port_meter_table
meter ingress_port_meter {
  type : bytes;
  instance_count : PORT_COUNT;
}

// Per-port ingress packet rate limiting.
table ingress_port_meter_table {
  reads {
    standard_metadata.ingress_port: exact;
    standard_metadata.egress_spec: exact;
    ethernet.etherType: exact;
    ipv4_base.dstAddr: ternary;
    arp.protoDstAddr: ternary;
    local_metadata.class_id: exact;
  }
  actions {
    meter_packet;
  }
}

//-----------------------------------
// Meter Stats
//-----------------------------------

counter meter_stats {
  type : packets;
  direct : ingress_port_meter_policy_table;
}

table ingress_port_meter_policy_table {
  reads {
    local_metadata.color : exact;
    local_metadata.ingress_meter_index : exact;
  }

  actions {
    meter_permit;
    meter_deny;
  }
}

//-----------------------------------
// Meter Control Flow
//-----------------------------------

control process_punt_packets {
  apply(punt_table);
  apply(ingress_port_meter_table) {
    hit {
      apply(ingress_port_meter_policy_table);
    }
  }
}

control ingress {
  if (valid(cpu_header)) {
    apply(process_cpu_header);
  } else {
    apply(class_id_assignment_table);
    apply(vrf_classifier_table);
    apply(l3_routing_classifier_table);
    ingress_lpm_forwarding();
    process_punt_packets();
  }
}

control egress {
}

