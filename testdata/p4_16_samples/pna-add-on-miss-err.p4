/*
Copyright 2020 Intel Corporation

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


typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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

struct empty_metadata_t {
}

// BEGIN:Counter_Example_Part1
typedef bit<48> ByteCounter_t;
typedef bit<32> PacketCounter_t;
typedef bit<80> PacketByteCounter_t;

const bit<32> NUM_PORTS = 4;
// END:Counter_Example_Part1


//////////////////////////////////////////////////////////////////////
// Struct types for holding user-defined collections of headers and
// metadata in the P4 developer's program.
//
// Note: The names of these struct types are completely up to the P4
// developer, as are their member fields, with the only restriction
// being that the structs intended to contain headers should only
// contain members whose types are header, header stack, or
// header_union.
//////////////////////////////////////////////////////////////////////

struct main_metadata_t {
    ExpireTimeProfileId_t timeout;
}

// User-defined struct containing all of those headers parsed in the
// main parser.
struct headers_t {
    ethernet_t ethernet;
    ipv4_t ipv4;
}

control PreControlImpl(
    in    headers_t  hdr,
    inout main_metadata_t meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {
        // Note: This program does not demonstrate all of the code
        // that would be necessary if you were implementing IPsec
        // packet decryption.

        // If it did, then this pre control implementation would do
        // one or more table lookups in order to determine whether the
        // packet was IPsec encapsulated, and if so, whether it is
        // part of a security association that was established by the
        // control plane software.

        // It would also likely perform anti-replay attack detection
        // on the IPsec sequence number, which is in the unencrypted
        // part of the packet.

        // Any headers parsed by the pre parser in pre_hdr will be
        // forgotten after this point.  The main parser will start
        // parsing over from the beginning, either on the same packet
        // if the inline extern block did nothing, or on the packet as
        // modified by the inline extern block.
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
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

// BEGIN:Counter_Example_Part2
control MainControlImpl(
    inout headers_t       hdr,           // from main parser
    inout main_metadata_t user_meta,     // from main parser, to "next block"
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{
    action next_hop(PortId_t vport) {
        send_to_port(vport);
    }
    action add_on_miss_action() {
        bit<32> tmp = 0;
        add_entry(action_name="next_hop", action_params = tmp, expire_time_profile_id = user_meta.timeout);
    }
    table ipv4_da {
        key = {
            hdr.ipv4.dstAddr: exact;
        }
        actions = {
            @tableonly next_hop;
            @defaultonly add_on_miss_action;
        }
        add_on_miss = false;
        const default_action = add_on_miss_action;
    }
    action next_hop2(PortId_t vport, bit<32> newAddr) {
        send_to_port(vport);
        hdr.ipv4.srcAddr = newAddr;
    }
    action add_on_miss_action2() {
        add_entry(action_name="next_hop2", action_params = {32w0, 32w1234}, expire_time_profile_id = user_meta.timeout);
    }
    table ipv4_da2 {
        key = {
            hdr.ipv4.dstAddr: exact;
        }
        actions = {
            @tableonly next_hop2;
            @defaultonly add_on_miss_action2;
        }
        add_on_miss = true;
        const default_action = add_on_miss_action2;
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_da.apply();
            ipv4_da2.apply();
        }
    }
}
// END:Counter_Example_Part2

control MainDeparserImpl(
    packet_out pkt,
    in    headers_t hdr,                // from main control
    in    main_metadata_t user_meta,    // from main control
    in    pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
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
