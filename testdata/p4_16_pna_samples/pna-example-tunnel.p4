/*
Copyright 2022 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <core.p4>
#include <pna.p4>

#define ETHERTYPE_IPV4  0x0800

typedef bit<48> ethernet_addr_t;
typedef bit<32> ipv4_addr_t;

header ethernet_t {
  ethernet_addr_t dst_addr;
  ethernet_addr_t src_addr;
  bit<16> ether_type;
}

header ipv4_t {
  bit<4> version;
  bit<4> ihl;
  bit<6> dscp; 
  bit<2> ecn;
  bit<16> total_len;
  bit<16> identification;
  bit<1> reserved;
  bit<1> do_not_fragment;
  bit<1> more_fragments;
  bit<13> frag_offset;
  bit<8> ttl;
  bit<8> protocol;
  bit<16> header_checksum;
  ipv4_addr_t src_addr;
  ipv4_addr_t dst_addr;
}

typedef bit<24> tunnel_id_t;
typedef bit<24> vni_id_t;
typedef bit<8> vrf_id_t;

struct headers_t {
  ethernet_t outer_ethernet;
  ipv4_t outer_ipv4;
  ethernet_t ethernet;
  ipv4_t ipv4;
}

struct tunnel_metadata_t {
  tunnel_id_t id;
  vni_id_t vni;
  bit<4> tun_type;
  bit<16> hash;
}

// Local metadata for each packet being processed.
struct local_metadata_t {
  ipv4_addr_t outer_ipv4_dst;
  tunnel_metadata_t tunnel;
}

parser packet_parser(packet_in packet, out headers_t headers,
                     inout local_metadata_t local_metadata,
                     in    pna_main_parser_input_metadata_t istd) {

  state start {
    transition parse_ethernet_otr;
  }

  state parse_ethernet_otr {
    packet.extract(headers.outer_ethernet);
    transition select(headers.outer_ethernet.ether_type) {
      ETHERTYPE_IPV4: parse_ipv4_otr;
      _:              accept;
    }
  }

  state parse_ipv4_otr {
    packet.extract(headers.outer_ipv4);
    transition select(headers.outer_ipv4.protocol) {
      _:                accept;
    }
  }

  state parse_ethernet {
    packet.extract(headers.ethernet);
    transition select(headers.ethernet.ether_type) {
      ETHERTYPE_IPV4: parse_ipv4;
      _:              accept;
    }
  }

  state parse_ipv4 {
    packet.extract(headers.ipv4);
    transition select(headers.ipv4.protocol) {
       _:                accept;
    }
  }
}  // parser packet_parser

control packet_deparser(packet_out packet, in headers_t headers,
                        in local_metadata_t local_metadata,
                        in    pna_main_output_metadata_t ostd) {
  apply {
    packet.emit(headers.outer_ethernet);
    packet.emit(headers.outer_ipv4);
    packet.emit(headers.ethernet);
    packet.emit(headers.ipv4);
  }
}  // control packet_deparser

bool RxPkt (in pna_main_input_metadata_t istd) {
    return (istd.direction == PNA_Direction_t.NET_TO_HOST);
}

control PreControlImpl(
    in    headers_t  hdr,
    inout local_metadata_t meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply { }
}

/* Tunnel Decap */
control tunnel_decap(inout headers_t hdr,
                     inout local_metadata_t local_metadata)
{
  action decap_outer_ipv4(tunnel_id_t tunnel_id) {
    local_metadata.tunnel.id = tunnel_id;
  }

  table ipv4_tunnel_term_table {
    key = {
      hdr.outer_ipv4.src_addr : exact @name("ipv4_src");
      hdr.outer_ipv4.dst_addr : exact @name("ipv4_dst");
      local_metadata.tunnel.tun_type : exact;
    }
    actions = {
      decap_outer_ipv4;
      @defaultonly NoAction;
    }
    const default_action = NoAction;
  }

  apply {
    ipv4_tunnel_term_table.apply();
  }
}  // control tunnel_decap

/* Tunnel Encap */
control tunnel_encap(inout headers_t hdr,
               inout local_metadata_t local_metadata,
               in pna_main_input_metadata_t istd)
{
  action set_tunnel(ipv4_addr_t dst_addr) {
    local_metadata.outer_ipv4_dst = dst_addr;
  }

  table set_tunnel_encap {
    key = {
      istd.input_port: exact;
    }
    actions = {
      set_tunnel;
      @defaultonly NoAction;
    }
    const default_action = NoAction;
    size = 256;
  }

  apply {
    set_tunnel_encap.apply();
  }
}


control main_control(inout headers_t hdr,
    inout local_metadata_t local_metadata,
    in pna_main_input_metadata_t istd,
    inout pna_main_output_metadata_t ostd)
{

  apply {

    if (RxPkt(istd)) {
        tunnel_decap.apply(hdr, local_metadata);
    } else {
        tunnel_encap.apply(hdr, local_metadata, istd);
    }

  }
}  // control main

PNA_NIC(packet_parser(), PreControlImpl(), main_control(), packet_deparser()) main;
