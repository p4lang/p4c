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

// User-defined struct containing all of those headers parsed in the
// pre parser.
struct pre_headers_t {
    ethernet_t ethernet;
    ipv4_t ipv4;
}

// User-defined struct containing metadata that is assigned values in
// the pre parser and/or control, and then carried by the PNA device
// from there to become input to the main parser and control.
struct pre_metadata_t {
    // empty for this skeleton
}

struct main_metadata_t {
    // empty for this skeleton
}

// User-defined struct containing all of those headers parsed in the
// main parser.
struct headers_t {
    ethernet_t ethernet;
    ipv4_t ipv4;
}

parser PreParserImpl (
    packet_in pkt,
    out pre_headers_t pre_hdr,
    out pre_metadata_t pre_user_meta,
    in  pna_pre_parser_input_metadata_t istd)
{
    state start {
        pkt.extract(pre_hdr.ethernet);
        transition select(pre_hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(pre_hdr.ipv4);
        transition accept;
    }
    // Note: This program does not demonstrate all of the code that
    // would be necessary if you were implementing IPsec packet
    // decryption.  If it did, then this pre parser implementation
    // would need to parse more headers, e.g. Authentication and ESP
    // headers, perhaps handling both IPsec transport and tunnel
    // modes.
}

// Note 1:

// pre_user_meta is an output from the pre parser, but it is optional
// for the pre parser to make any assignments to any of its members.
// It is only done this way in case it is helpful to carry metadata
// fields about the packet from the pre parser to the pre control,
// that are not in parsed headers.

// pre_user_meta is also an output from the pre control, and an input
// to the main parser.  This allows the pre parser and/or control to
// record metadata about the packet that will be carried with the
// packet from the pre parser/control to main.

control PreControlImpl(
    in    pre_headers_t pre_hdr,           // output from pre parser
    inout pre_metadata_t pre_user_meta,    // See Note 1
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
    in    pre_metadata_t  pre_meta,
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
    in    pre_metadata_t  pre_user_meta, // from pre control
    inout headers_t       hdr,           // from main parser
    inout main_metadata_t user_meta,     // from main parser, to "next block"
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{
    Counter<ByteCounter_t, PortId_t>(NUM_PORTS, PNA_CounterType_t.BYTES)
        port_bytes_in;
    DirectCounter<PacketByteCounter_t>(PNA_CounterType_t.PACKETS_AND_BYTES)
        per_prefix_pkt_byte_count;

    action next_hop(VportId_t vport) {
        per_prefix_pkt_byte_count.count();
        ostd.dest_vport = vport;
    }
    action default_route_drop() {
        per_prefix_pkt_byte_count.count();
        ostd.drop = true;
    }
    table ipv4_da_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            next_hop;
            default_route_drop;
        }
        const default_action = default_route_drop;
        // table ipv4_da_lpm owns this DirectCounter instance
        pna_direct_counter = per_prefix_pkt_byte_count;
    }
    apply {
        port_bytes_in.count(istd.input_port);
        if (hdr.ipv4.isValid()) {
            ipv4_da_lpm.apply();
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
    PreParserImpl(),
    PreControlImpl(),
    MainParserImpl(),
    MainControlImpl(),
    MainDeparserImpl()) main;
// END:Package_Instantiation_Example
