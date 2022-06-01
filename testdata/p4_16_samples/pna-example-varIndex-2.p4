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
#include "pna.p4"

#define MAX_LAYERS 2
typedef bit<48>  EthernetAddress;

enum bit<16> ether_type_t {
    TPID = 0x8100,
    IPV4 = 0x0800,
    IPV6 = 0x86DD
}

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header vlan_tag_h {
    bit<3>        pcp;
    bit<1>        cfi;
    bit<12>       vid;
    ether_type_t  ether_type;
}

struct headers_t {
    ethernet_t ethernet;
    vlan_tag_h[MAX_LAYERS]      vlan_tag;
}

struct main_metadata_t {
    bit<2> depth;
    bit<16> ethType;
}

control PreControlImpl(
    in    headers_t  hdrs,
    inout main_metadata_t meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {
    }
}

parser MainParserImpl(
    packet_in pkt,
    out   headers_t       hdrs,
    inout main_metadata_t meta,
    in    pna_main_parser_input_metadata_t istd)
{
    state start {
        meta.depth = MAX_LAYERS - 1;
        pkt.extract(hdrs.ethernet);
        transition select(hdrs.ethernet.etherType) {
            ether_type_t.TPID :  parse_vlan_tag;
            default: accept;
        }
    }
    state parse_vlan_tag {
        pkt.extract(hdrs.vlan_tag.next);
        meta.depth = meta.depth - 1;
        transition select(hdrs.vlan_tag.last.ether_type) {
            ether_type_t.TPID :  parse_vlan_tag;
            default: accept;
        }
    }

}

control MainControlImpl(
    inout headers_t       hdrs,           // from main parser
    inout main_metadata_t meta,     // from main parser, to "next block"
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{
    action execute() {
        hdrs.vlan_tag[meta.depth].vid = hdrs.vlan_tag[0].vid;
    }
    action execute_1() {
         drop_packet();
    }
		
    table stub {
        key = {
              hdrs.vlan_tag[meta.depth].vid : exact;
        }

        actions = {
		execute;
        }
	const default_action = execute;
        size=1000000;
    }

    table stub1 {
        key = {
              hdrs.ethernet.etherType : exact;
        }

        actions = {
		execute_1;
        }
	const default_action = execute_1;
        size=1000000;
    }

    apply {
        switch (hdrs.vlan_tag[meta.depth].vid) {
            12w1: { stub.apply();}
            12w2: { stub1.apply();}
        }
    }
}

control MainDeparserImpl(
    packet_out pkt,
    in    headers_t hdr,                // from main control
    in    main_metadata_t user_meta,    // from main control
    in    pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.vlan_tag[0]);
        pkt.emit(hdr.vlan_tag[1]);
    }
}

// BEGIN:Package_Instantiation_Example
PNA_NIC(
    MainParserImpl(),
    PreControlImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    // Hoping to make this optional parameter later, but not supported
    // by p4c yet.
    //, PreParserImpl()
    ) main;
// END:Package_Instantiation_Example
