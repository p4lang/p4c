/*
Copyright (c) 2015-2016 by The Board of Trustees of the Leland
Stanford Junior University.  All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


 Author: Lisa Yan (yanlisa@stanford.edu)
*/

/* Should be 16 tables
 * Do we need check_ipv6_size though? it kinda sucks
 */
#define ROUTABLE_SIZE 64
#define SMAC_VLAN_SIZE 160000
#define VRF_SIZE 40000
#define CHECK_IPV6_SIZE 1
#define IPv6_PREFIX_SIZE 1000
#define CHECK_UCAST_IPV4_SIZE 1
#define IPV4_FORWARDING_SIZE 160000
#define IPV6_FORWARDING_SIZE 5000
#define NEXT_HOP_SIZE 41250
#define IPV4_XCAST_FORWARDING_SIZE 16000
#define IPV6_XCAST_FORWARDING_SIZE 1000
#define URPF_V4_SIZE 160000
#define URPF_V6_SIZE 5000
#define IGMP_SNOOPING_SIZE 16000
#define DMAC_VLAN_SIZE 160000
#define ACL_SIZE 80000

#define VRF_BIT_WIDTH 12

parser start {
    return parse_ethernet;
}

#define ETHERTYPE_BF_FABRIC    0x9000
#define ETHERTYPE_BF_SFLOW     0x9001
#define ETHERTYPE_VLAN         0x8100, 0x9100
#define ETHERTYPE_MPLS         0x8847
#define ETHERTYPE_IPV4         0x0800
#define ETHERTYPE_IPV6         0x86dd
#define ETHERTYPE_ARP          0x0806
#define ETHERTYPE_RARP         0x8035
#define ETHERTYPE_NSH          0x894f
#define ETHERTYPE_ETHERNET     0x6558
#define ETHERTYPE_TRILL        0x22f3
#define ETHERTYPE_VNTAG        0x8926

#define IPV4_MULTICAST_MAC 0x01005E
#define IPV6_MULTICAST_MAC 0x3333

/* Tunnel types */
#define INGRESS_TUNNEL_TYPE_NONE               0
#define INGRESS_TUNNEL_TYPE_VXLAN              1
#define INGRESS_TUNNEL_TYPE_GRE                2
#define INGRESS_TUNNEL_TYPE_GENEVE             3
#define INGRESS_TUNNEL_TYPE_NVGRE              4
#define INGRESS_TUNNEL_TYPE_MPLS_L2VPN         5
#define INGRESS_TUNNEL_TYPE_MPLS_L3VPN         8

#define PARSE_ETHERTYPE                                    \
        ETHERTYPE_VLAN : parse_vlan;                       \
        ETHERTYPE_MPLS : parse_mpls;                       \
        ETHERTYPE_IPV4 : parse_ipv4;                       \
        ETHERTYPE_IPV6 : parse_ipv6;                       \
        ETHERTYPE_ARP : parse_arp_rarp;                    \
        ETHERTYPE_RARP : parse_arp_rarp;                   \
        default: ingress

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        PARSE_ETHERTYPE;
    }
}

#define VLAN_DEPTH 1
header vlan_tag_t vlan_tag_[VLAN_DEPTH];

parser parse_vlan {
    extract(vlan_tag_[next]);
    return select(latest.etherType) {
        PARSE_ETHERTYPE;
    }
}

#define MPLS_DEPTH 1
header mpls_t mpls[MPLS_DEPTH];

parser parse_mpls {
    extract(mpls[next]);
    return select(latest.bos) {
        0 : parse_mpls;
        default: ingress;
    }
}

#define IP_PROTOCOLS_ICMP              1
#define IP_PROTOCOLS_IGMP              2
#define IP_PROTOCOLS_IPV4              4
#define IP_PROTOCOLS_TCP               6
#define IP_PROTOCOLS_UDP               17
#define IP_PROTOCOLS_IPV6              41
#define IP_PROTOCOLS_GRE               47
#define IP_PROTOCOLS_IPSEC_ESP         50
#define IP_PROTOCOLS_IPSEC_AH          51
#define IP_PROTOCOLS_ICMPV6            58
#define IP_PROTOCOLS_EIGRP             88
#define IP_PROTOCOLS_OSPF              89
#define IP_PROTOCOLS_PIM               103
#define IP_PROTOCOLS_VRRP              112

#define IP_PROTOCOLS_IPHL_ICMP         0x501
#define IP_PROTOCOLS_IPHL_IPV4         0x504
#define IP_PROTOCOLS_IPHL_TCP          0x506
#define IP_PROTOCOLS_IPHL_UDP          0x511
#define IP_PROTOCOLS_IPHL_IPV6         0x529
#define IP_PROTOCOLS_IPHL_GRE          0x52f

header ipv4_t ipv4;

field_list ipv4_checksum_list {
        ipv4.version;
        ipv4.ihl;
        ipv4.diffserv;
        ipv4.totalLen;
        ipv4.identification;
        ipv4.flags;
        ipv4.fragOffset;
        ipv4.ttl;
        ipv4.protocol;
        ipv4.srcAddr;
        ipv4.dstAddr;
}

field_list_calculation ipv4_checksum {
    input {
        ipv4_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4.hdrChecksum  {
    verify ipv4_checksum if (ipv4.ihl == 5);
    update ipv4_checksum if (ipv4.ihl == 5);
}

parser parse_ipv4 {
    extract(ipv4);
    return select(latest.fragOffset, latest.ihl, latest.protocol) {
        IP_PROTOCOLS_IPHL_ICMP : parse_icmp;
        IP_PROTOCOLS_IPHL_TCP : parse_tcp;
        IP_PROTOCOLS_IPHL_UDP : parse_udp;
        IP_PROTOCOLS_IPHL_GRE : parse_gre;
        default: ingress;
    }
}

header ipv6_t ipv6;

parser parse_ipv6 {
    extract(ipv6);
    return select(latest.nextHdr) {
        IP_PROTOCOLS_ICMPV6 : parse_icmp;
        IP_PROTOCOLS_TCP : parse_tcp;
        IP_PROTOCOLS_UDP : parse_udp;
        IP_PROTOCOLS_GRE : parse_gre;
        default: ingress;
    }
}

header icmp_t icmp;

parser parse_icmp {
    extract(icmp);
    return ingress;
}

#define TCP_PORT_BGP                   179
#define TCP_PORT_MSDP                  639

header tcp_t tcp;
parser parse_tcp {
    extract(tcp);
    return ingress;
}

header udp_t udp;

parser parse_udp {
    extract(udp);
    return ingress;
}

#define GRE_PROTOCOLS_NVGRE            0x20006558
#define GRE_PROTOCOLS_ERSPAN_T3        0x22EB   /* Type III version 2 */

header gre_t gre;

parser parse_gre {
    extract(gre);
    return ingress;
}

#define ARP_PROTOTYPES_ARP_RARP_IPV4 0x0800

header arp_rarp_t arp_rarp;

parser parse_arp_rarp {
    extract(arp_rarp);
    return ingress;
}

header_type hop_metadata_t {
    fields {
        vrf : VRF_BIT_WIDTH;
        ipv6_prefix : 64;
        next_hop_index : 16;
        mcast_grp : 16;
        urpf_fail : 1;
        drop_reason : 8;
    }
}

metadata hop_metadata_t hop_metadata;

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
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

header_type mpls_t {
    fields {
        label : 20;
        exp : 3;
        bos : 1;
        ttl : 8;
    }
}

header_type ipv4_t {
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
        dstAddr: 32;
    }
}

header_type ipv6_t {
    fields {
        version : 4;
        trafficClass : 8;
        flowLabel : 20;
        payloadLen : 16;
        nextHdr : 8;
        hopLimit : 8;
        srcAddr : 128;
        dstAddr : 128;
    }
}

header_type icmp_t {
    fields {
        type_ : 8;
        code : 8;
        hdrChecksum : 16;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 3;
        ecn : 3;
        ctrl : 6;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        length_ : 16;
        checksum : 16;
    }
}

header_type gre_t {
    fields {
        C : 1;
        R : 1;
        K : 1;
        S : 1;
        s : 1;
        recurse : 3;
        flags : 5;
        ver : 3;
        proto : 16;
    }
}

header_type arp_rarp_t {
    fields {
        hwType : 16;
        protoType : 16;
        hwAddrLen : 8;
        protoAddrLen : 8;
        opcode : 16;
    }
}

action on_hit() {
}
action on_miss() {
}
action nop() {
}

action set_vrf(vrf) {
    modify_field(hop_metadata.vrf, vrf);
}

action set_ipv6_prefix_ucast(ipv6_prefix){
    modify_field(hop_metadata.ipv6_prefix, ipv6_prefix);
}

action set_ipv6_prefix_xcast(ipv6_prefix){
    modify_field(hop_metadata.ipv6_prefix, ipv6_prefix);
}

action set_next_hop(dst_index) {
    modify_field(hop_metadata.next_hop_index, dst_index);
}

action set_ethernet_addr(smac, dmac) {
    modify_field(ethernet.srcAddr, smac);
    modify_field(ethernet.dstAddr, dmac);
}

action set_multicast_replication_list(mc_index) {
    modify_field(hop_metadata.mcast_grp, mc_index);
}

action set_urpf_check_fail() {
    modify_field(hop_metadata.urpf_fail, 1);
}

action urpf_check_fail() {
    set_urpf_check_fail();
    drop();
}

action set_egress_port(e_port) {
    modify_field(standard_metadata.egress_spec, e_port);
}

action action_drop(drop_reason) {
    modify_field(hop_metadata.drop_reason, drop_reason);
    drop();
}

/* Table 1:  Routable
 * Matches on VLAN, port #, DMAC and Ethertype to determine if this frame
 * is routable (addressed to the router).
 * Set Routable (field or metadata)
 */
table routable {
    reads {
        vlan_tag_[0] : valid;
        vlan_tag_[0].vid : exact;
        standard_metadata.ingress_port : exact;
        ethernet.dstAddr : exact;
        ethernet.etherType : exact;
    }
    actions {
        on_hit;
        on_miss;
    }
    size : ROUTABLE_SIZE;
}

/* Table 2:  SMAC/VLAN
 * Matches on the SMAC/VLAN from the frame (not affected by routing)
 */
table smac_vlan {
    reads {
        ethernet.srcAddr : exact;
        standard_metadata.ingress_port : exact;
    }
    actions {
        nop; /* FIX */
    }
    size : SMAC_VLAN_SIZE;
}

/* Table 3:  VRF
 * Matches {VLAN, SMAC} produces a 8-bit VRF
 * Set Vrf (field or metadata)
 */
table vrf {
    reads {
        vlan_tag_[0] : valid;
        vlan_tag_[0].vid : exact;
        ethernet.srcAddr : exact;
        standard_metadata.ingress_port : exact;
    }
    actions {
        set_vrf;
    }
    size : VRF_SIZE;
}

/* Table 4: Check IPv6
 * Checks if IPv6 header is valid.
 */
table check_ipv6 {
    reads {
        ipv6 : valid;
    }
    actions {
        on_hit;
        on_miss;
    }
    size : CHECK_IPV6_SIZE;
}

/* Table 5:  IPv6 Prefix
 * Matches on Dest IPv6[127:64], produces a 16-bit prefix
 * Set Ipv6_Prefix (field or metadata)
 */
table ipv6_prefix {
    reads {
        ipv6.dstAddr : lpm;
    }
    actions {
        set_ipv6_prefix_ucast;
        set_ipv6_prefix_xcast;
    }
    size : IPv6_PREFIX_SIZE;
}

/* Table 6: IPv4 Ucast
 * checks if an address falls into xcast category
 * and redirects to xcast table if needed.
 */
table check_ucast_ipv4 {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
        nop; /* FIX */
    }
    size : CHECK_UCAST_IPV4_SIZE;
}

/* Table 7:  IPv4 Forwarding
 * Match on {VRF, DestIPv4}, produce a NextHop Index
 * Ucast only.
 */
table ipv4_forwarding {
    reads {
        hop_metadata.vrf : exact;
        ipv4.dstAddr : lpm;
    }
    actions {
        set_next_hop;
    }
    size : IPV4_FORWARDING_SIZE;
}

/* Table 8:  IPv6 Forwarding
 * Match on {VRF, v6prefix, destipv6[63:0]}, produce a NextHop Index
 */
table ipv6_forwarding {
    reads {
        hop_metadata.vrf : exact;
        hop_metadata.ipv6_prefix : exact;
        ipv6.dstAddr : lpm;
    }
    actions {
        set_next_hop;
    }
    size : IPV6_FORWARDING_SIZE;
}

/* Table 9: Next Hop
 * Use NextHop Index to set src/dst MAC addresses.
 */
table next_hop {
    reads {
        hop_metadata.next_hop_index : exact;
    }
    actions {
        set_ethernet_addr;
    }
    size : NEXT_HOP_SIZE;
}

/* Table 10:  Multicast IPv4 Forwarding
 * Match on {VLAN, DestIPv4, SourceIPv4}, produce a multicast replication list
 * , flood set, and/or a PIM assert
 */
table ipv4_xcast_forwarding {
    reads {
        vlan_tag_[0].vid : exact;
        ipv4.dstAddr : lpm;
        ipv4.srcAddr : lpm;
        standard_metadata.ingress_port : exact;
    }
    actions {
        set_multicast_replication_list;
    }
    size : IPV4_XCAST_FORWARDING_SIZE;
}

/* Table 11:  Multicast IPv6 Forwarding
 * Match on {VLAN, DestIPv6, SourceIPv6}, produce a multicast replication list
 * , flood set, and/or a PIM assert
 */
table ipv6_xcast_forwarding {
    reads {
        vlan_tag_[0].vid : exact;
        ipv6.dstAddr : lpm;
        ipv6.srcAddr : lpm;
        standard_metadata.ingress_port : exact;
    }
    actions {
        set_multicast_replication_list;
    }
    size : IPV6_XCAST_FORWARDING_SIZE;
}

/* Table 12:  uRPF IPv4 Check
 * Match on {VLAN, SourceIPv4}, action is NOP
 * Default action is to drop
 */
table urpf_v4 {
    reads {
        vlan_tag_[0].vid : exact;
        standard_metadata.ingress_port : exact;
        ipv4.srcAddr : exact;
    }
    actions {
        urpf_check_fail;
        nop;
    }
    size : URPF_V4_SIZE;
}

/* Table 13:  uRPF IPv6 Check
 * Match on {VLAN, SourceIPv6}, action is NOP
 * Default action is to drop
 */
table urpf_v6 {
    reads {
        vlan_tag_[0].vid : exact;
        standard_metadata.ingress_port : exact;
        ipv6.srcAddr : exact;
    }
    actions {
        urpf_check_fail;
        nop;
    }
    size : URPF_V6_SIZE;
}

/* Table 14:  IGMP Snooping
 */
table igmp_snooping {
    reads {
        ipv4.dstAddr : lpm;
        vlan_tag_[0].vid : exact;
        standard_metadata.ingress_port : exact;
    }
    actions {
        nop; /* FIX */
    }
    size : IGMP_SNOOPING_SIZE;
}

/* Table 15:  DMAC/VLAN
 * Matches on the DMAC/VLAN either from the frame, or from the result of the
 * route (NextHop DMAC/VLAN)
 */
table dmac_vlan {
    reads {
        ethernet.dstAddr : exact;
        vlan_tag_[0].vid : exact;
        standard_metadata.ingress_port : exact;
    }
    actions {
        set_egress_port;
    }
    size : DMAC_VLAN_SIZE;
}

/* Table 16:  ACLs
 *   Generically match on a set of ingress conditions
 *   Generically associate some action (permit/deny/police/mirror/trap/SetField/
 *   etc.)
 * ????????????????????? only ipv4??
 */
table acl {
    reads {
        ipv4.srcAddr : lpm;
        ipv4.dstAddr : lpm;
        ethernet.srcAddr : exact;
        ethernet.dstAddr : exact;
    }
    actions {
        action_drop;
    }
    size : ACL_SIZE;
}


control process_ip {
    apply(vrf);
    apply(check_ipv6) {
        on_hit {
            apply(ipv6_prefix) {
                set_ipv6_prefix_ucast {
                    apply(urpf_v6);
                    apply(ipv6_forwarding);
                }
                set_ipv6_prefix_xcast {
                    apply(ipv6_xcast_forwarding);
                }
            }
        }
        on_miss {
            apply(check_ucast_ipv4) {
                on_hit {
                    apply(urpf_v4);
                    apply(ipv4_forwarding);
                }
                on_miss {
                    apply(igmp_snooping);
                    apply(ipv4_xcast_forwarding);
                }
            }
        }
    }
    apply(acl); /* Perhaps right before routable */
    apply(next_hop);
}

control ingress {
    apply(smac_vlan);
    apply(routable) {
        on_hit {
            process_ip();
        }
    }
    apply(dmac_vlan);
}

control egress {
}
