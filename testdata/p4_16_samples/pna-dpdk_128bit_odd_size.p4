/*
Copyright 2024 Andy Fingerhut

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
#include "dpdk/pna.p4"


typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header custom_t {
    bit<1> padding;
    bit<1> f1;
    bit<2> f2;
    bit<4> f4;
    bit<8> f8;
    bit<16> f16;
    bit<32> f32;
    bit<64> f64;
    bit<128> f128;
}

struct main_metadata_t {
}

struct headers_t {
    ethernet_t ethernet;
    custom_t custom;
}

control PreControlImpl(
    in    headers_t  hdr,
    inout main_metadata_t meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {
    }
}

parser MainParserImpl(
    packet_in pkt,
    out   headers_t       hdr,
    inout main_metadata_t main_meta,
    in    pna_main_parser_input_metadata_t istd)
{
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x08ff: parse_custom;
            default: accept;
        }
    }
    state parse_custom {
        pkt.extract(hdr.custom);
        transition accept;
    }
}

control MainControlImpl(
    inout headers_t       hdr,
    inout main_metadata_t user_meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{
    action a1(bit<73> x) {
        hdr.custom.f128 = (bit<128>) x;
    }
    table t1 {
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        actions = {
            @tableonly a1;
            @defaultonly NoAction;
        }
        size = 512;
        const default_action = NoAction();
    }
    apply {
        if (hdr.custom.isValid()) {
            t1.apply();
        }
    }
}

control MainDeparserImpl(
    packet_out pkt,
    in    headers_t hdr,
    in    main_metadata_t user_meta,
    in    pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.custom);
    }
}

PNA_NIC(
    MainParserImpl(),
    PreControlImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    // Hoping to make this optional parameter later, but not supported
    // by p4c yet.
    //, PreParserImpl()
    ) main;

