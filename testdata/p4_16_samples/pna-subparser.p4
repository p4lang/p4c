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

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<8>  version_ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<16> flags_fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct main_metadata_t {
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser CommonParser(packet_in pkt,
                    out headers_t hdr,
                    inout main_metadata_t meta)
{
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

parser MainParserImpl(
          packet_in                        pkt,
    out   headers_t                        hdr,
    inout main_metadata_t                  meta,
    in    pna_main_parser_input_metadata_t istd)
{
    CommonParser() p;

    state start {
        transition packet_in_parsing;
    }

    state packet_in_parsing {
        p.apply(pkt, hdr, meta);
        transition accept;
    }
}

control PreControlImpl(
    in    headers_t                 hdr,
    inout main_metadata_t           meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {}
}

control MainControlImpl(
    inout headers_t                  hdr,
    inout main_metadata_t            meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{
    apply {
    }
}

control MainDeparserImpl(
       packet_out                 pkt,
    in headers_t                  hdr,
    in main_metadata_t            meta,
    in pna_main_output_metadata_t ostd)
{
    apply {
    }
}

PNA_NIC(
    MainParserImpl(),
    PreControlImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    ) main;
