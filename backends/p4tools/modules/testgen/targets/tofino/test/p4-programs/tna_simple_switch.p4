/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

/* -*- P4_16 -*- */





#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

@command_line("--disable-parse-max-depth-limit")

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<12> vlan_id_t;
typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x800;
const ether_type_t ETHERTYPE_ARP = 16w0x806;
const ether_type_t ETHERTYPE_VLAN = 16w0x8100;
const ether_type_t ETHERTYPE_IPV6 = 16w0x86dd;
typedef bit<8> ip_protocol_t;
const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_IPV4 = 4;
const ip_protocol_t IP_PROTOCOLS_TCP = 6;
const ip_protocol_t IP_PROTOCOLS_UDP = 17;
const ip_protocol_t IP_PROTOCOLS_IPV6 = 41;
const ip_protocol_t IP_PROTOCOLS_SRV6 = 43;
const ip_protocol_t IP_PROTOCOLS_NONXT = 59;
typedef bit<16> udp_port_t;
const udp_port_t UDP_PORT_GTPC = 2123;
const udp_port_t PORT_GTPU = 2152;
const udp_port_t UDP_PORT_VXLAN = 4789;
header ethernet_h {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16>    ether_type;
}

header vlan_tag_h {
    bit<3>    pcp;
    bit<1>    cfi;
    vlan_id_t vid;
    bit<16>   ether_type;
}

header mpls_h {
    bit<20> label;
    bit<3>  exp;
    bit<1>  bos;
    bit<8>  ttl;
}

header ipv4_h {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     total_len;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     frag_offset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdr_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header ipv6_h {
    bit<4>      version;
    bit<8>      traffic_class;
    bit<20>     flow_label;
    bit<16>     payload_len;
    bit<8>      next_hdr;
    bit<8>      hop_limit;
    ipv6_addr_t src_addr;
    ipv6_addr_t dst_addr;
}

header tcp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> hdr_length;
    bit<16> checksum;
}

header icmp_h {
    bit<8>  type_;
    bit<8>  code;
    bit<16> hdr_checksum;
}

header srh_h {
    bit<8>  next_hdr;
    bit<8>  hdr_ext_len;
    bit<8>  routing_type;
    bit<8>  segment_left;
    bit<8>  last_entry;
    bit<8>  flags;
    bit<16> tag;
}

header srh_segment_list_t {
    bit<128> sid;
}

header gtpu_h {
    bit<3>  version;
    bit<1>  pt;
    bit<1>  reserved;
    bit<1>  e;
    bit<1>  s;
    bit<1>  pn;
    bit<8>  message_type;
    bit<16> message_len;
    bit<32> teid;
}

typedef bit<16> bd_t;
typedef bit<16> vrf_t;
typedef bit<16> nexthop_t;
typedef bit<16> ifindex_t;
typedef bit<8> bypass_t;
const bypass_t BYPASS_L2 = 8w0x1;
const bypass_t BYPASS_L3 = 8w0x2;
const bypass_t BYPASS_ACL = 8w0x4;
const bypass_t BYPASS_ALL = 8w0xff;
typedef bit<128> srv6_sid_t;
struct srv6_metadata_t {
    srv6_sid_t sid;
    bit<16>    rewrite;
    bool       psp;
    bool       usp;
    bool       decap;
    bool       encap;
}

struct ingress_metadata_t {
    bool            checksum_err;
    bd_t            bd;
    vrf_t           vrf;
    nexthop_t       nexthop;
    ifindex_t       ifindex;
    ifindex_t       egress_ifindex;
    bypass_t        bypass;
    srv6_metadata_t srv6;
}

struct egress_metadata_t {
    srv6_metadata_t srv6;
}

header bridged_metadata_t {
    bit<16> rewrite;
    bit<1>  psp;
    bit<1>  usp;
    bit<1>  decap;
    bit<1>  encap;
    bit<4>  pad;
}

struct lookup_fields_t {
    mac_addr_t  mac_src_addr;
    mac_addr_t  mac_dst_addr;
    bit<16>     mac_type;
    bit<4>      ip_version;
    bit<8>      ip_proto;
    bit<8>      ip_ttl;
    bit<8>      ip_dscp;
    bit<20>     ipv6_flow_label;
    ipv4_addr_t ipv4_src_addr;
    ipv4_addr_t ipv4_dst_addr;
    ipv6_addr_t ipv6_src_addr;
    ipv6_addr_t ipv6_dst_addr;
}

struct header_t {
    bridged_metadata_t    bridged_md;
    ethernet_h            ethernet;
    vlan_tag_h            vlan_tag;
    ipv4_h                ipv4;
    ipv6_h                ipv6;
    srh_h                 srh;
    srh_segment_list_t[5] srh_segment_list;
    tcp_h                 tcp;
    udp_h                 udp;
    gtpu_h                gtpu;
    ethernet_h            inner_ethernet;
    ipv6_h                inner_ipv6;
    srh_h                 inner_srh;
    srh_segment_list_t[5] inner_srh_segment_list;
    ipv4_h                inner_ipv4;
    tcp_h                 inner_tcp;
    udp_h                 inner_udp;
}

parser TofinoIngressParser(packet_in pkt, out ingress_intrinsic_metadata_t ig_intr_md) {
    state start {
        pkt.extract(ig_intr_md);
        transition select(ig_intr_md.resubmit_flag) {
            1: parse_resubmit;
            0: parse_port_metadata;
        }
    }
    state parse_resubmit {
        transition reject;
    }
    state parse_port_metadata {
        pkt.advance(PORT_METADATA_SIZE);
        transition accept;
    }
}

parser TofinoEgressParser(packet_in pkt, out egress_intrinsic_metadata_t eg_intr_md) {
    state start {
        pkt.extract(eg_intr_md);
        transition accept;
    }
}

parser SwitchIngressParser(packet_in pkt, out header_t hdr, out ingress_metadata_t ig_md, out ingress_intrinsic_metadata_t ig_intr_md) {
    Checksum() ipv4_checksum;
    TofinoIngressParser() tofino_parser;
    state start {
        tofino_parser.apply(pkt, ig_intr_md);
        transition parse_ethernet;
    }
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_IPV6: parse_ipv6;
            ETHERTYPE_VLAN: parse_vlan;
            default: accept;
        }
    }
    state parse_vlan {
        pkt.extract(hdr.vlan_tag);
        transition select(hdr.vlan_tag.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_IPV6: parse_ipv6;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        ipv4_checksum.add(hdr.ipv4);
        ig_md.checksum_err = ipv4_checksum.verify();
        transition select(hdr.ipv4.protocol) {
            IP_PROTOCOLS_TCP: parse_tcp;
            IP_PROTOCOLS_UDP: parse_udp;
            default: accept;
        }
    }
    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition select(hdr.ipv6.next_hdr) {
            IP_PROTOCOLS_TCP: parse_tcp;
            IP_PROTOCOLS_UDP: parse_udp;
            IP_PROTOCOLS_SRV6: parse_srh;
            IP_PROTOCOLS_IPV6: parse_inner_ipv6;
            IP_PROTOCOLS_IPV4: parse_inner_ipv4;
            default: accept;
        }
    }
    state parse_srh {
        pkt.extract(hdr.srh);
        transition parse_srh_segment_0;
    }
    state parse_srh_segment_0 {
        pkt.extract(hdr.srh_segment_list[0]);
        transition select(hdr.srh.last_entry) {
            0: parse_srh_next_header;
            default: parse_srh_segment_1;
        }
    }
    state set_active_segment_0 {
        transition parse_srh_segment_1;
    }
    state parse_srh_segment_1 {
        pkt.extract(hdr.srh_segment_list[1]);
        transition select(hdr.srh.last_entry) {
            1: parse_srh_next_header;
            default: parse_srh_segment_2;
        }
    }
    state set_active_segment_1 {
        transition parse_srh_segment_2;
    }
    state parse_srh_segment_2 {
        pkt.extract(hdr.srh_segment_list[2]);
        transition select(hdr.srh.last_entry) {
            2: parse_srh_next_header;
            default: parse_srh_segment_3;
        }
    }
    state set_active_segment_2 {
        transition parse_srh_segment_3;
    }
    state parse_srh_segment_3 {
        pkt.extract(hdr.srh_segment_list[3]);
        transition select(hdr.srh.last_entry) {
            3: parse_srh_next_header;
            default: parse_srh_segment_4;
        }
    }
    state set_active_segment_3 {
        transition parse_srh_segment_4;
    }
    state parse_srh_segment_4 {
        pkt.extract(hdr.srh_segment_list[4]);
        transition parse_srh_next_header;
    }
    state set_active_segment_4 {
        ig_md.srv6.sid = hdr.srh_segment_list[4].sid;
        transition parse_srh_next_header;
    }
    state parse_srh_next_header {
        transition select(hdr.srh.next_hdr) {
            IP_PROTOCOLS_IPV6: parse_inner_ipv6;
            IP_PROTOCOLS_IPV4: parse_inner_ipv4;
            IP_PROTOCOLS_SRV6: parse_inner_srh;
            IP_PROTOCOLS_NONXT: accept;
            default: reject;
        }
    }
    state parse_udp {
        pkt.extract(hdr.udp);
        transition select(hdr.udp.dst_port) {
            PORT_GTPU: parse_gtpu;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }
    state parse_gtpu {
        pkt.extract(hdr.gtpu);
        bit<4> version = pkt.lookahead<bit<4>>();
        transition select(version) {
            4w4: parse_inner_ipv4;
            4w6: parse_inner_ipv6;
        }
    }
    state parse_inner_ipv4 {
        pkt.extract(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.protocol) {
            IP_PROTOCOLS_TCP: parse_inner_tcp;
            IP_PROTOCOLS_UDP: parse_inner_udp;
            default: accept;
        }
    }
    state parse_inner_ipv6 {
        pkt.extract(hdr.inner_ipv6);
        transition select(hdr.inner_ipv6.next_hdr) {
            IP_PROTOCOLS_TCP: parse_inner_tcp;
            IP_PROTOCOLS_UDP: parse_inner_udp;
            IP_PROTOCOLS_SRV6: parse_inner_srh;
            default: accept;
        }
    }
    state parse_inner_srh {
        pkt.extract(hdr.inner_srh);
        transition accept;
    }
    state parse_inner_udp {
        pkt.extract(hdr.inner_udp);
        transition select(hdr.inner_udp.dst_port) {
            default: accept;
        }
    }
    state parse_inner_tcp {
        pkt.extract(hdr.inner_tcp);
        transition accept;
    }
}

control SwitchIngressDeparser(packet_out pkt, inout header_t hdr, in ingress_metadata_t ig_md, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
    apply {
        pkt.emit(hdr.bridged_md);
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.ipv6);
        pkt.emit(hdr.srh);
        pkt.emit(hdr.srh_segment_list);
        pkt.emit(hdr.udp);
        pkt.emit(hdr.tcp);
        pkt.emit(hdr.gtpu);
        pkt.emit(hdr.inner_ipv4);
        pkt.emit(hdr.inner_ipv6);
        pkt.emit(hdr.inner_srh);
        pkt.emit(hdr.inner_srh_segment_list);
        pkt.emit(hdr.inner_udp);
        pkt.emit(hdr.inner_tcp);
    }
}

parser SwitchEgressParser(packet_in pkt, out header_t hdr, out egress_metadata_t eg_md, out egress_intrinsic_metadata_t eg_intr_md) {
    TofinoEgressParser() tofino_parser;
    state start {
        tofino_parser.apply(pkt, eg_intr_md);
        transition parse_bridged_metadata;
    }
    state parse_bridged_metadata {
        pkt.extract(hdr.bridged_md);
        eg_md.srv6.rewrite = hdr.bridged_md.rewrite;
        eg_md.srv6.psp = (bool)hdr.bridged_md.psp;
        eg_md.srv6.usp = (bool)hdr.bridged_md.psp;
        eg_md.srv6.encap = (bool)hdr.bridged_md.encap;
        eg_md.srv6.decap = (bool)hdr.bridged_md.decap;
        transition parse_ethernet;
    }
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_IPV6: parse_ipv6;
            ETHERTYPE_VLAN: parse_vlan;
            default: accept;
        }
    }
    state parse_vlan {
        pkt.extract(hdr.vlan_tag);
        transition select(hdr.vlan_tag.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_IPV6: parse_ipv6;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IP_PROTOCOLS_UDP: parse_udp;
            IP_PROTOCOLS_TCP: parse_tcp;
            default: accept;
        }
    }
    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition select(hdr.ipv6.next_hdr) {
            IP_PROTOCOLS_UDP: parse_udp;
            IP_PROTOCOLS_TCP: parse_tcp;
            IP_PROTOCOLS_SRV6: parse_srh;
            IP_PROTOCOLS_IPV6: parse_inner_ipv6;
            IP_PROTOCOLS_IPV4: parse_inner_ipv4;
            default: accept;
        }
    }
    state parse_srh {
        pkt.extract(hdr.srh);
        transition parse_srh_segment_0;
    }
    state parse_srh_segment_0 {
        pkt.extract(hdr.srh_segment_list[0]);
        transition select(hdr.srh.last_entry) {
            0: parse_srh_next_header;
            default: parse_srh_segment_1;
        }
    }
    state parse_srh_segment_1 {
        pkt.extract(hdr.srh_segment_list[1]);
        transition select(hdr.srh.last_entry) {
            1: parse_srh_next_header;
            default: parse_srh_segment_2;
        }
    }
    state parse_srh_segment_2 {
        pkt.extract(hdr.srh_segment_list[2]);
        transition select(hdr.srh.last_entry) {
            2: parse_srh_next_header;
            default: parse_srh_segment_3;
        }
    }
    state parse_srh_segment_3 {
        pkt.extract(hdr.srh_segment_list[3]);
        transition select(hdr.srh.last_entry) {
            3: parse_srh_next_header;
            default: parse_srh_segment_4;
        }
    }
    state parse_srh_segment_4 {
        pkt.extract(hdr.srh_segment_list[4]);
        transition parse_srh_next_header;
    }
    state parse_srh_next_header {
        transition select(hdr.srh.next_hdr) {
            IP_PROTOCOLS_IPV6: parse_inner_ipv6;
            IP_PROTOCOLS_IPV4: parse_inner_ipv4;
            IP_PROTOCOLS_SRV6: parse_inner_srh;
            IP_PROTOCOLS_NONXT: accept;
            default: reject;
        }
    }
    state parse_udp {
        pkt.extract(hdr.udp);
        transition select(hdr.udp.dst_port) {
            PORT_GTPU: parse_gtpu;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition select(hdr.tcp.dst_port) {
            PORT_GTPU: parse_gtpu;
            default: accept;
        }
    }
    state parse_gtpu {
        pkt.extract(hdr.gtpu);
        bit<4> version = pkt.lookahead<bit<4>>();
        transition select(version) {
            4w4: parse_inner_ipv4;
            4w6: parse_inner_ipv6;
        }
    }
    state parse_inner_ipv4 {
        pkt.extract(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.protocol) {
            IP_PROTOCOLS_UDP: parse_inner_udp;
            IP_PROTOCOLS_TCP: parse_inner_tcp;
            default: accept;
        }
    }
    state parse_inner_ipv6 {
        pkt.extract(hdr.inner_ipv6);
        transition select(hdr.inner_ipv6.next_hdr) {
            IP_PROTOCOLS_UDP: parse_inner_udp;
            IP_PROTOCOLS_TCP: parse_inner_tcp;
            IP_PROTOCOLS_SRV6: parse_inner_srh;
            default: accept;
        }
    }
    state parse_inner_srh {
        pkt.extract(hdr.inner_srh);
        transition accept;
    }
    state parse_inner_udp {
        pkt.extract(hdr.inner_udp);
        transition accept;
    }
    state parse_inner_tcp {
        pkt.extract(hdr.inner_tcp);
        transition accept;
    }
}

control SwitchEgressDeparser(packet_out pkt, inout header_t hdr, in egress_metadata_t eg_md, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) {
    Checksum() ipv4_checksum;
    apply {
        hdr.ipv4.hdr_checksum = ipv4_checksum.update({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.src_addr, hdr.ipv4.dst_addr });
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.ipv6);
        pkt.emit(hdr.srh);
        pkt.emit(hdr.srh_segment_list);
        pkt.emit(hdr.udp);
        pkt.emit(hdr.gtpu);
        pkt.emit(hdr.inner_ipv4);
        pkt.emit(hdr.inner_ipv6);
        pkt.emit(hdr.inner_srh);
        pkt.emit(hdr.inner_srh_segment_list);
        pkt.emit(hdr.inner_udp);
    }
}

control SRv6(inout header_t hdr, inout ingress_metadata_t ig_md, inout lookup_fields_t lkp) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) cnt1;
    action t() {
    }
    action t_insert(srv6_sid_t s1, bit<16> rewrite_index) {
        lkp.ipv6_dst_addr = s1;
        hdr.ipv6.dst_addr = s1;
        ig_md.srv6.rewrite = rewrite_index;
    }
    action t_insert_red(srv6_sid_t s1, bit<16> rewrite_index) {
        lkp.ipv6_dst_addr = s1;
        hdr.ipv6.dst_addr = s1;
        ig_md.srv6.rewrite = rewrite_index;
    }
    action t_encaps(srv6_sid_t s1, bit<20> flow_label, bit<16> rewrite_index) {
        lkp.ipv6_dst_addr = s1;
        lkp.ipv6_flow_label = flow_label;
        ig_md.srv6.encap = true;
        ig_md.srv6.rewrite = rewrite_index;
    }
    action t_encaps_red(srv6_sid_t s1, bit<20> flow_label, bit<16> rewrite_index) {
        lkp.ipv6_dst_addr = s1;
        lkp.ipv6_flow_label = flow_label;
        ig_md.srv6.encap = true;
        ig_md.srv6.rewrite = rewrite_index;
    }
    action t_encaps_l2() {
    }
    action t_encaps_l2_red() {
    }
    action drop() {
        cnt1.count();
    }
    action end() {
        lkp.ipv6_dst_addr = ig_md.srv6.sid;
        hdr.srh.segment_left = hdr.srh.segment_left - 1;
        hdr.ipv6.dst_addr = ig_md.srv6.sid;
        cnt1.count();
    }
    action end_x(nexthop_t nexthop) {
        ig_md.nexthop = nexthop;
        ig_md.bypass = BYPASS_L3;
        hdr.srh.segment_left = hdr.srh.segment_left - 1;
        hdr.ipv6.dst_addr = ig_md.srv6.sid;
        cnt1.count();
    }
    action end_t() {
        cnt1.count();
    }
    action end_dx2(ifindex_t ifindex) {
        ig_md.egress_ifindex = ifindex;
        ig_md.bypass = BYPASS_ALL;
        ig_md.srv6.decap = true;
        cnt1.count();
    }
    action end_dx2v() {
        cnt1.count();
    }
    action end_dt2u(bd_t bd) {
        lkp.mac_src_addr = hdr.inner_ethernet.src_addr;
        lkp.mac_dst_addr = hdr.inner_ethernet.dst_addr;
        ig_md.srv6.decap = true;
        ig_md.bd = bd;
        cnt1.count();
    }
    action end_dt2m() {
        lkp.mac_src_addr = hdr.inner_ethernet.src_addr;
        ig_md.srv6.decap = true;
        cnt1.count();
    }
    action end_dx6(nexthop_t nexthop) {
        ig_md.nexthop = nexthop;
        ig_md.bypass = BYPASS_L3;
        ig_md.srv6.decap = true;
        cnt1.count();
    }
    action end_dx4(nexthop_t nexthop) {
        ig_md.nexthop = nexthop;
        ig_md.bypass = BYPASS_L3;
        ig_md.srv6.decap = true;
        cnt1.count();
    }
    action end_dt4(vrf_t vrf) {
        ig_md.vrf = vrf;
        lkp.ipv4_src_addr = hdr.inner_ipv4.src_addr;
        lkp.ipv4_dst_addr = hdr.inner_ipv4.dst_addr;
        lkp.ip_proto = hdr.inner_ipv4.protocol;
        ig_md.srv6.decap = true;
        cnt1.count();
    }
    action end_dt6(vrf_t vrf) {
        ig_md.vrf = vrf;
        lkp.ipv6_src_addr = hdr.inner_ipv6.src_addr;
        lkp.ipv6_dst_addr = hdr.inner_ipv6.dst_addr;
        lkp.ip_proto = hdr.inner_ipv6.next_hdr;
        lkp.ipv6_flow_label = hdr.inner_ipv6.flow_label;
        ig_md.srv6.decap = true;
        cnt1.count();
    }
    action end_dt46() {
    }
    action end_b6(srv6_sid_t s1, bit<16> rewrite_index) {
        lkp.ipv6_dst_addr = s1;
        hdr.ipv6.dst_addr = s1;
        ig_md.srv6.rewrite = rewrite_index;
        cnt1.count();
    }
    action end_b6_red(srv6_sid_t s1, bit<16> rewrite_index) {
        lkp.ipv6_dst_addr = s1;
        hdr.ipv6.dst_addr = s1;
        ig_md.srv6.rewrite = rewrite_index;
        cnt1.count();
    }
    action end_b6_encaps(bit<16> rewrite_index) {
        lkp.ipv6_dst_addr = ig_md.srv6.sid;
        hdr.srh.segment_left = hdr.srh.segment_left - 1;
        hdr.ipv6.dst_addr = ig_md.srv6.sid;
        ig_md.srv6.rewrite = rewrite_index;
        ig_md.srv6.encap = true;
        cnt1.count();
    }
    action end_b6_encaps_red(bit<16> rewrite_index) {
        lkp.ipv6_dst_addr = ig_md.srv6.sid;
        hdr.srh.segment_left = hdr.srh.segment_left - 1;
        hdr.ipv6.dst_addr = ig_md.srv6.sid;
        ig_md.srv6.rewrite = rewrite_index;
        ig_md.srv6.encap = true;
        cnt1.count();
    }
    action end_bm() {
    }
    action end_s() {
    }
    action end_map(srv6_sid_t sid) {
        lkp.ipv6_dst_addr = sid;
        hdr.ipv6.dst_addr = sid;
        cnt1.count();
    }
    action end_m_gtp6_d(srv6_sid_t s1, bit<16> rewrite_index) {
        lkp.ipv6_dst_addr = s1;
        ig_md.srv6.rewrite = rewrite_index;
        ig_md.srv6.decap = true;
        ig_md.srv6.encap = true;
        cnt1.count();
    }
    action end_m_gtp6_e(bit<16> rewrite_index) {
        hdr.srh.segment_left = hdr.srh.segment_left - 1;
        lkp.ipv6_dst_addr = ig_md.srv6.sid;
        ig_md.srv6.rewrite = rewrite_index;
        ig_md.srv6.decap = true;
        ig_md.srv6.encap = true;
        cnt1.count();
    }
    action end_m_gtp4_d() {
        cnt1.count();
    }
    action end_limit() {
        cnt1.count();
    }
    action t_map() {
    }
    action end_as(nexthop_t nexthop) {
        ig_md.nexthop = nexthop;
        ig_md.bypass = BYPASS_L3;
        ig_md.srv6.decap = true;
        cnt1.count();
    }
    action end_ad(nexthop_t nexthop) {
        ig_md.nexthop = nexthop;
        ig_md.bypass = BYPASS_L3;
        ig_md.srv6.decap = true;
        cnt1.count();
    }
    action static_proxy(srv6_sid_t s1, bit<16> rewrite_index) {
        lkp.ipv6_dst_addr = s1;
        ig_md.srv6.rewrite = rewrite_index;
        ig_md.srv6.encap = true;
        cnt1.count();
    }
    action dynamic_proxy() {
        cnt1.count();
    }
    table local_sid {
        key = {
            hdr.ipv6.dst_addr   : ternary;
            hdr.ipv6.next_hdr   : ternary;
            hdr.srh.isValid()   : ternary;
            hdr.srh.segment_left: ternary;
            hdr.srh.next_hdr    : ternary;
            ig_md.ifindex       : ternary;
        }
        actions = {
            @defaultonly NoAction;
            drop;
            end;
            end_x;
            end_t;
            end_dx2;
            end_dx2v;
            end_dt2u;
            end_dt2m;
            end_dx4;
            end_dx6;
            end_dt4;
            end_dt6;
            end_b6;
            end_b6_red;
            end_b6_encaps;
            end_b6_encaps_red;
            end_map;
            end_m_gtp6_d;
            end_m_gtp6_e;
            end_m_gtp4_d;
            end_limit;
            end_as;
            end_ad;
            static_proxy;
            dynamic_proxy;
        }
        const default_action = NoAction;
        counters = cnt1;
    }
    table transit_ {
        key = {
            hdr.ipv6.dst_addr: lpm;
        }
        actions = {
            t;
            t_insert;
            t_insert_red;
            t_encaps;
            t_encaps_red;
            t_encaps_l2;
            t_encaps_l2_red;
        }
    }
    apply {
        if (hdr.ipv6.isValid()) {
            if (!local_sid.apply().hit) {
                transit_.apply();
            }
        }
    }
}

control SRv6Decap(in srv6_metadata_t srv6_md, inout header_t hdr) {
    action decap_inner_udp() {
        hdr.udp = hdr.inner_udp;
        hdr.inner_udp.setInvalid();
        hdr.gtpu.setInvalid();
    }
    action decap_inner_tcp() {
        hdr.tcp = hdr.inner_tcp;
        hdr.inner_tcp.setInvalid();
        hdr.gtpu.setInvalid();
    }
    action decap_inner_unknown() {
        hdr.gtpu.setInvalid();
    }
    table decap_inner_l4 {
        key = {
            hdr.inner_udp.isValid(): exact;
            hdr.inner_tcp.isValid(): exact;
        }
        actions = {
            decap_inner_udp;
            decap_inner_tcp;
            decap_inner_unknown;
        }
        const default_action = decap_inner_unknown;
        const entries = {
                        (true, false) : decap_inner_udp();
                        (false, true) : decap_inner_tcp();
        }
    }
    action remove_srh_header() {
        hdr.srh.setInvalid();
        hdr.srh_segment_list[0].setInvalid();
        hdr.srh_segment_list[1].setInvalid();
        hdr.srh_segment_list[2].setInvalid();
        hdr.srh_segment_list[3].setInvalid();
        hdr.srh_segment_list[4].setInvalid();
    }
    action remove_inner_srh_header() {
        hdr.inner_srh.setInvalid();
        hdr.inner_srh_segment_list[0].setInvalid();
        hdr.inner_srh_segment_list[1].setInvalid();
        hdr.inner_srh_segment_list[2].setInvalid();
        hdr.inner_srh_segment_list[3].setInvalid();
        hdr.inner_srh_segment_list[4].setInvalid();
    }
    action copy_srh_header() {
        hdr.srh = hdr.inner_srh;
        hdr.srh_segment_list[0] = hdr.inner_srh_segment_list[0];
        hdr.srh_segment_list[1] = hdr.inner_srh_segment_list[1];
        hdr.srh_segment_list[2] = hdr.inner_srh_segment_list[2];
        hdr.srh_segment_list[3] = hdr.inner_srh_segment_list[3];
    }
    action decap_inner_non_ip() {
        hdr.ethernet = hdr.inner_ethernet;
        hdr.inner_ethernet.setInvalid();
        hdr.ipv6.setInvalid();
        remove_srh_header();
    }
    action decap_inner_ipv6() {
        hdr.ethernet.ether_type = ETHERTYPE_IPV6;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        remove_srh_header();
    }
    action decap_inner_ipv6_srh() {
        hdr.ethernet.ether_type = ETHERTYPE_IPV6;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.inner_ipv6.setInvalid();
        copy_srh_header();
        remove_inner_srh_header();
    }
    table decap_inner_ip {
        key = {
            hdr.inner_ipv6.isValid(): exact;
            hdr.inner_srh.isValid() : exact;
        }
        actions = {
            NoAction;
            decap_inner_non_ip;
            decap_inner_ipv6;
            decap_inner_ipv6_srh;
        }
        const default_action = NoAction;
        const entries = {
            (false, false) : decap_inner_non_ip();
            (true, false) : decap_inner_ipv6();
            (true, true) : decap_inner_ipv6_srh();
        }
    }
    apply {
        if (srv6_md.decap) {
            decap_inner_l4.apply();
            decap_inner_ip.apply();
        }
    }
}

control SRv6Encap(in srv6_metadata_t srv6_md, inout header_t hdr) {
    bit<8> proto;
    bit<16> len;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) cnt2;
    action encap_outer_udp() {
        hdr.inner_udp = hdr.udp;
        hdr.udp.setInvalid();
    }
    action encap_outer_tcp() {
        hdr.inner_tcp = hdr.tcp;
        hdr.tcp.setInvalid();
    }
    action encap_outer_unknown() {
    }
    table encap_outer_l4 {
        key = {
            hdr.udp.isValid(): exact;
            hdr.tcp.isValid(): exact;
        }
        actions = {
            encap_outer_tcp;
            encap_outer_udp;
            encap_outer_unknown;
        }
        const default_action = encap_outer_unknown;
        const entries = {
                        (true, false) : encap_outer_udp();
                        (false, true) : encap_outer_tcp();
        }
    }
    action remove_srh_header() {
        hdr.srh.setInvalid();
        hdr.srh_segment_list[0].setInvalid();
        hdr.srh_segment_list[1].setInvalid();
        hdr.srh_segment_list[2].setInvalid();
        hdr.srh_segment_list[3].setInvalid();
    }
    action copy_srh_header() {
        hdr.inner_srh = hdr.srh;
        hdr.inner_srh_segment_list[0] = hdr.srh_segment_list[0];
        hdr.inner_srh_segment_list[1] = hdr.srh_segment_list[1];
        hdr.inner_srh_segment_list[2] = hdr.srh_segment_list[2];
        hdr.inner_srh_segment_list[3] = hdr.srh_segment_list[3];
    }
    action encap_outer_ipv6() {
        hdr.inner_ipv6 = hdr.ipv6;
        hdr.ipv6.setInvalid();
        len = hdr.ipv6.payload_len + 16w40;
        proto = IP_PROTOCOLS_IPV6;
    }
    action encap_outer_ipv6_srh() {
        hdr.inner_ipv6 = hdr.ipv6;
        copy_srh_header();
        hdr.ipv6.setInvalid();
        len = hdr.ipv6.payload_len + 16w40;
        proto = IP_PROTOCOLS_IPV6;
    }
    action push_outer_srh() {
        copy_srh_header();
        remove_srh_header();
        proto = IP_PROTOCOLS_SRV6;
    }
    action no_encap() {
        proto = hdr.ipv6.next_hdr;
    }
    table encap_outer_ip {
        key = {
            srv6_md.encap     : exact;
            hdr.ipv6.isValid(): exact;
            hdr.srh.isValid() : exact;
        }
        actions = {
            no_encap;
            encap_outer_ipv6;
            encap_outer_ipv6_srh;
            push_outer_srh;
        }
        const entries = {
            (false, true, true) : push_outer_srh();
            (true, true, false) : encap_outer_ipv6();
            (true, true, true) : encap_outer_ipv6_srh();
        }
        const default_action = no_encap;
    }
    action insert_srh_header(in bit<8> next_hdr, in bit<8> hdr_ext_len, in bit<8> last_entry, in bit<8> segment_left) {
        hdr.srh.setValid();
        hdr.srh.next_hdr = next_hdr;
        hdr.srh.hdr_ext_len = hdr_ext_len;
        hdr.srh.routing_type = 0x4;
        hdr.srh.segment_left = segment_left;
        hdr.srh.last_entry = last_entry;
        hdr.srh.flags = 0;
        hdr.srh.tag = 0;
    }
    action rewrite_ipv6(ipv6_addr_t src_addr, ipv6_addr_t dst_addr, bit<8> next_hdr) {
        hdr.ethernet.ether_type = ETHERTYPE_IPV6;
        hdr.ipv6.setValid();
        hdr.ipv6.version = 4w6;
        hdr.ipv6.traffic_class = 0;
        hdr.ipv6.flow_label = 0;
        hdr.ipv6.payload_len = len;
        hdr.ipv6.next_hdr = next_hdr;
        hdr.ipv6.hop_limit = 8w64;
        hdr.ipv6.src_addr = src_addr;
        hdr.ipv6.dst_addr = dst_addr;
    }
    table ipv6_rewrite {
        key = {
            srv6_md.rewrite: exact;
        }
        actions = {
            NoAction;
            rewrite_ipv6;
        }
        const default_action = NoAction;
    }
    action rewrite_srh_0() {
        hdr.ipv6.next_hdr = proto;
        cnt2.count();
    }
    action rewrite_srh_1(bit<8> segment_left, srv6_sid_t s1) {
        hdr.ipv6.payload_len = hdr.ipv6.payload_len + 16w24;
        hdr.ipv6.next_hdr = IP_PROTOCOLS_SRV6;
        insert_srh_header(proto, 8w2, 8w0, segment_left);
        hdr.srh_segment_list[0].setValid();
        hdr.srh_segment_list[0].sid = s1;
        cnt2.count();
    }
    action rewrite_srh_2(bit<8> segment_left, srv6_sid_t s1, srv6_sid_t s2) {
        hdr.ipv6.payload_len = hdr.ipv6.payload_len + 16w40;
        hdr.ipv6.next_hdr = IP_PROTOCOLS_SRV6;
        insert_srh_header(proto, 8w4, 8w1, segment_left);
        hdr.srh_segment_list[0].setValid();
        hdr.srh_segment_list[1].setValid();
        hdr.srh_segment_list[0].sid = s2;
        hdr.srh_segment_list[1].sid = s1;
        cnt2.count();
    }
    action rewrite_srh_3(bit<8> segment_left, srv6_sid_t s1, srv6_sid_t s2, srv6_sid_t s3) {
        hdr.ipv6.payload_len = hdr.ipv6.payload_len + 16w56;
        hdr.ipv6.next_hdr = IP_PROTOCOLS_SRV6;
        insert_srh_header(proto, 8w6, 8w2, segment_left);
        hdr.srh_segment_list[0].setValid();
        hdr.srh_segment_list[1].setValid();
        hdr.srh_segment_list[2].setValid();
        hdr.srh_segment_list[0].sid = s3;
        hdr.srh_segment_list[1].sid = s2;
        hdr.srh_segment_list[2].sid = s1;
        cnt2.count();
    }
    action rewrite_srh_4(bit<8> segment_left, srv6_sid_t s1, srv6_sid_t s2, srv6_sid_t s3, srv6_sid_t s4) {
        hdr.ipv6.payload_len = hdr.ipv6.payload_len + 16w72;
        hdr.ipv6.next_hdr = IP_PROTOCOLS_SRV6;
        insert_srh_header(proto, 8w8, 8w3, segment_left);
        hdr.srh_segment_list[0].setValid();
        hdr.srh_segment_list[1].setValid();
        hdr.srh_segment_list[2].setValid();
        hdr.srh_segment_list[3].setValid();
        hdr.srh_segment_list[0].sid = s4;
        hdr.srh_segment_list[1].sid = s3;
        hdr.srh_segment_list[2].sid = s2;
        hdr.srh_segment_list[3].sid = s1;
        cnt2.count();
    }
    action rewrite_srh_5(bit<8> segment_left, srv6_sid_t s1, srv6_sid_t s2, srv6_sid_t s3, srv6_sid_t s4, srv6_sid_t s5) {
        hdr.ipv6.payload_len = hdr.ipv6.payload_len + 16w88;
        hdr.ipv6.next_hdr = IP_PROTOCOLS_SRV6;
        insert_srh_header(proto, 8w10, 8w4, segment_left);
        hdr.srh_segment_list[0].setValid();
        hdr.srh_segment_list[1].setValid();
        hdr.srh_segment_list[2].setValid();
        hdr.srh_segment_list[3].setValid();
        hdr.srh_segment_list[4].setValid();
        hdr.srh_segment_list[0].sid = s5;
        hdr.srh_segment_list[1].sid = s4;
        hdr.srh_segment_list[2].sid = s3;
        hdr.srh_segment_list[3].sid = s2;
        hdr.srh_segment_list[4].sid = s1;
        cnt2.count();
    }
    table srh_rewrite {
        key = {
            srv6_md.rewrite: exact;
        }
        actions = {
            @defaultonly NoAction;
            rewrite_srh_0;
            rewrite_srh_1;
            rewrite_srh_2;
            rewrite_srh_3;
        }
        const default_action = NoAction;
        counters = cnt2;
    }
    apply {
        if (srv6_md.rewrite != 0) {
            encap_outer_l4.apply();
            encap_outer_ip.apply();
        }
        ipv6_rewrite.apply();
        srh_rewrite.apply();
    }
}

control PktValidation(in header_t hdr, out lookup_fields_t lkp) {
    const bit<32> table_size = 512;
    action malformed_pkt() {
    }
    action valid_pkt_untagged() {
        lkp.mac_src_addr = hdr.ethernet.src_addr;
        lkp.mac_dst_addr = hdr.ethernet.dst_addr;
        lkp.mac_type = hdr.ethernet.ether_type;
    }
    action valid_pkt_tagged() {
        lkp.mac_src_addr = hdr.ethernet.src_addr;
        lkp.mac_dst_addr = hdr.ethernet.dst_addr;
        lkp.mac_type = hdr.vlan_tag.ether_type;
    }
    table validate_ethernet {
        key = {
            hdr.ethernet.src_addr : ternary;
            hdr.ethernet.dst_addr : ternary;
            hdr.vlan_tag.isValid(): ternary;
        }
        actions = {
            malformed_pkt;
            valid_pkt_untagged;
            valid_pkt_tagged;
        }
        size = table_size;
    }
    action valid_ipv4_pkt() {
        lkp.ip_version = 4w4;
        lkp.ip_dscp = hdr.ipv4.diffserv;
        lkp.ip_proto = hdr.ipv4.protocol;
        lkp.ip_ttl = hdr.ipv4.ttl;
        lkp.ipv4_src_addr = hdr.ipv4.src_addr;
        lkp.ipv4_dst_addr = hdr.ipv4.dst_addr;
    }
    table validate_ipv4 {
        key = {
            hdr.ipv4.version: ternary;
            hdr.ipv4.ihl    : ternary;
            hdr.ipv4.ttl    : ternary;
        }
        actions = {
            valid_ipv4_pkt;
            malformed_pkt;
        }
        size = table_size;
    }
    action valid_ipv6_pkt() {
        lkp.ip_version = 4w6;
        lkp.ip_dscp = hdr.ipv6.traffic_class;
        lkp.ip_proto = hdr.ipv6.next_hdr;
        lkp.ip_ttl = hdr.ipv6.hop_limit;
        lkp.ipv6_src_addr = hdr.ipv6.src_addr;
        lkp.ipv6_dst_addr = hdr.ipv6.dst_addr;
    }
    table validate_ipv6 {
        key = {
            hdr.ipv6.version  : ternary;
            hdr.ipv6.hop_limit: ternary;
        }
        actions = {
            valid_ipv6_pkt;
            malformed_pkt;
        }
        size = table_size;
    }
    apply {
        validate_ethernet.apply();
        if (hdr.ipv4.isValid()) {
            validate_ipv4.apply();
        } else if (hdr.ipv6.isValid()) {
            validate_ipv6.apply();
        }
    }
}

control PortMapping(in PortId_t port, in vlan_tag_h vlan_tag, inout ingress_metadata_t ig_md)(bit<32> port_vlan_table_size, bit<32> bd_table_size) {
    ActionProfile(bd_table_size) bd_action_profile;
    action set_port_attributes(ifindex_t ifindex) {
        ig_md.ifindex = ifindex;
    }
    table port_mapping {
        key = {
            port: exact;
        }
        actions = {
            set_port_attributes;
        }
    }
    action set_bd_attributes(bd_t bd, vrf_t vrf) {
        ig_md.bd = bd;
        ig_md.vrf = vrf;
    }
    table port_vlan_to_bd_mapping {
        key = {
            ig_md.ifindex     : exact;
            vlan_tag.isValid(): ternary;
            vlan_tag.vid      : ternary;
        }
        actions = {
            NoAction;
            set_bd_attributes;
        }
        const default_action = NoAction;
        implementation = bd_action_profile;
        size = port_vlan_table_size;
    }
    apply {
        port_mapping.apply();
        port_vlan_to_bd_mapping.apply();
    }
}

control MAC(in mac_addr_t dst_addr, in bd_t bd, out ifindex_t egress_ifindex)(bit<32> mac_table_size) {
    action dmac_miss() {
        egress_ifindex = 16w0xffff;
    }
    action dmac_hit(ifindex_t ifindex) {
        egress_ifindex = ifindex;
    }
    table dmac {
        key = {
            bd      : exact;
            dst_addr: exact;
        }
        actions = {
            dmac_miss;
            dmac_hit;
        }
        const default_action = dmac_miss;
        size = mac_table_size;
    }
    apply {
        dmac.apply();
    }
}

control FIB(in ipv4_addr_t dst_addr, in vrf_t vrf, out nexthop_t nexthop)(bit<32> host_table_size, bit<32> lpm_table_size) {
    action fib_hit(nexthop_t nexthop_index) {
        nexthop = nexthop_index;
    }
    action fib_miss() {
    }
    table fib {
        key = {
            vrf     : exact;
            dst_addr: exact;
        }
        actions = {
            fib_miss;
            fib_hit;
        }
        const default_action = fib_miss;
        size = host_table_size;
    }
    table fib_lpm {
        key = {
            vrf     : exact;
            dst_addr: lpm;
        }
        actions = {
            fib_miss;
            fib_hit;
        }
        const default_action = fib_miss;
        size = lpm_table_size;
    }
    apply {
        if (!fib.apply().hit) {
            fib_lpm.apply();
        }
    }
}

control FIBv6(in ipv6_addr_t dst_addr, in vrf_t vrf, out nexthop_t nexthop)(bit<32> host_table_size, bit<32> lpm_table_size) {
    action fib_hit(nexthop_t nexthop_index) {
        nexthop = nexthop_index;
    }
    action fib_miss() {
    }
    table fib {
        key = {
            vrf     : exact;
            dst_addr: exact;
        }
        actions = {
            fib_miss;
            fib_hit;
        }
        const default_action = fib_miss;
        size = host_table_size;
    }
    table fib_lpm {
        key = {
            vrf     : exact;
            dst_addr: lpm;
        }
        actions = {
            fib_miss;
            fib_hit;
        }
        const default_action = fib_miss;
        size = lpm_table_size;
    }
    apply {
        if (!fib.apply().hit) {
            fib_lpm.apply();
        }
    }
}

control Nexthop(in lookup_fields_t lkp, inout header_t hdr, inout ingress_metadata_t ig_md)(bit<32> table_size) {
    bool routed;
    Hash<bit<32>>(HashAlgorithm_t.CRC32) sel_hash;
    ActionProfile(2048) ecmp_selector_ap;

    ActionSelector(ecmp_selector_ap, // action profile
                   sel_hash, // hash extern
                   SelectorMode_t.FAIR, // Selector algorithm
                   200, // max group size
                   100 // max number of groups
                   ) ecmp_selector;
    action set_nexthop_attributes(bd_t bd, mac_addr_t dmac) {
        routed = true;
        ig_md.bd = bd;
        hdr.ethernet.dst_addr = dmac;
    }
    table ecmp {
        key = {
            ig_md.nexthop      : exact;
            lkp.ipv6_src_addr  : selector;
            lkp.ipv6_dst_addr  : selector;
            lkp.ipv6_flow_label: selector;
            lkp.ipv4_src_addr  : selector;
            lkp.ipv4_dst_addr  : selector;
        }
        actions = {
            NoAction;
            set_nexthop_attributes;
        }
        const default_action = NoAction;
        size = table_size;
        implementation = ecmp_selector;
    }
    table nexthop {
        key = {
            ig_md.nexthop: exact;
        }
        actions = {
            NoAction;
            set_nexthop_attributes;
        }
        const default_action = NoAction;
        size = table_size;
    }
    action rewrite_ipv4() {
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }
    action rewrite_ipv6() {
        hdr.ipv6.hop_limit = hdr.ipv6.hop_limit - 1;
    }
    table ip_rewrite {
        key = {
            hdr.ipv4.isValid(): exact;
            hdr.ipv6.isValid(): exact;
        }
        actions = {
            rewrite_ipv4;
            rewrite_ipv6;
        }
        const entries = {
                        (true, false) : rewrite_ipv4();
                        (false, true) : rewrite_ipv6();
        }
    }
    action rewrite_smac(mac_addr_t smac) {
        hdr.ethernet.src_addr = smac;
    }
    table smac_rewrite {
        key = {
            ig_md.bd: exact;
        }
        actions = {
            NoAction;
            rewrite_smac;
        }
        const default_action = NoAction;
    }
    apply {
        routed = false;
        switch (nexthop.apply().action_run) {
            NoAction: {
                ecmp.apply();
            }
        }
        if (routed) {
            ip_rewrite.apply();
            smac_rewrite.apply();
        }
    }
}

control LAG(in lookup_fields_t lkp, in ifindex_t ifindex, out PortId_t egress_port) {
    Hash<bit<32>>(HashAlgorithm_t.CRC32) sel_hash;
    ActionProfile(2048) lag_selector_ap;

    ActionSelector(lag_selector_ap, // action profile
                   sel_hash, // hash extern
                   SelectorMode_t.FAIR, // Selector algorithm
                   200, // max group size
                   100 // max number of groups
                   ) lag_selector;
    action set_port(PortId_t port) {
        egress_port = port;
    }
    action lag_miss() {
    }
    table lag {
        key = {
            ifindex            : exact;
            lkp.ipv6_src_addr  : selector;
            lkp.ipv6_dst_addr  : selector;
            lkp.ipv6_flow_label: selector;
            lkp.ipv4_src_addr  : selector;
            lkp.ipv4_dst_addr  : selector;
        }
        actions = {
            lag_miss;
            set_port;
        }
        const default_action = lag_miss;
        size = 1024;
        implementation = lag_selector;
    }
    apply {
        lag.apply();
    }
}

control SwitchIngress(inout header_t hdr, inout ingress_metadata_t ig_md, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    PortMapping(1024, 1024) port_mapping;
    PktValidation() pkt_validation;
    SRv6() srv6;
    MAC(1024) mac;
    FIB(1024, 1024) fib;
    FIBv6(1024, 1024) fibv6;
    Nexthop(1024) nexthop;
    LAG() lag;
    lookup_fields_t lkp;
    action add_bridged_md() {
        hdr.bridged_md.setValid();
        hdr.bridged_md.rewrite = ig_md.srv6.rewrite;
        hdr.bridged_md.psp = (bit<1>)ig_md.srv6.psp;
        hdr.bridged_md.usp = (bit<1>)ig_md.srv6.usp;
        hdr.bridged_md.decap = (bit<1>)ig_md.srv6.decap;
        hdr.bridged_md.encap = (bit<1>)ig_md.srv6.encap;
    }
    action rmac_hit() {
    }
    table rmac {
        key = {
            lkp.mac_dst_addr: exact;
        }
        actions = {
            NoAction;
            rmac_hit;
        }
        const default_action = NoAction;
        size = 1024;
    }
    apply {
        port_mapping.apply(ig_intr_md.ingress_port, hdr.vlan_tag, ig_md);
        pkt_validation.apply(hdr, lkp);
        switch (rmac.apply().action_run) {
            rmac_hit: {
                ig_md.bypass = 0;
                srv6.apply(hdr, ig_md, lkp);
                if (!(ig_md.bypass & BYPASS_L3 != 0)) {
                    if (lkp.ip_version == 4w4) {
                        fib.apply(lkp.ipv4_dst_addr, ig_md.vrf, ig_md.nexthop);
                    } else if (lkp.ip_version == 4w6) {
                        fibv6.apply(lkp.ipv6_dst_addr, ig_md.vrf, ig_md.nexthop);
                    }
                }
            }
        }
        nexthop.apply(lkp, hdr, ig_md);
        if (!(ig_md.bypass & BYPASS_L2 != 0)) {
            mac.apply(hdr.ethernet.dst_addr, ig_md.bd, ig_md.egress_ifindex);
        }
        lag.apply(lkp, ig_md.egress_ifindex, ig_tm_md.ucast_egress_port);
        add_bridged_md();
    }
}

control SwitchEgress(inout header_t hdr, inout egress_metadata_t eg_md, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_intr_from_prsr, inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr, inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
    SRv6Decap() decap;
    SRv6Encap() encap;
    apply {
        decap.apply(eg_md.srv6, hdr);
        encap.apply(eg_md.srv6, hdr);
    }
}

Pipeline(SwitchIngressParser(), SwitchIngress(), SwitchIngressDeparser(), SwitchEgressParser(), SwitchEgress(), SwitchEgressDeparser()) pipe;

Switch(pipe) main;