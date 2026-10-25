/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

// This program processes packets composed of an Ethernet and
// an IPv4 header, performing forwarding based on the
// destination IP address

typedef bit<48>  EthernetAddress;
typedef bit<32>  IPv4Address;

// standard Ethernet header
header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

// IPv4 header without options
header Ipv4_h {
    bit<4>       version;
    bit<4>       ihl;
    bit<8>       diffserv;
    bit<16>      totalLen;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      fragOffset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdrChecksum;
    IPv4Address  srcAddr;
    IPv4Address  dstAddr;
}

// Parser section

// List of all recognized headers
struct Headers {
    Ethernet_h ethernet;
    Ipv4_h     ip;
}

struct Metadata {
}

parser P(packet_in b,
         out Headers p,
         inout Metadata meta,
         inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

// match-action pipeline section

control Ing(inout Headers headers,
            inout Metadata meta,
            inout standard_metadata_t standard_meta) {

    register<bit<8>>(32w2) debug;

    apply {
        bit<8> n = 8w0b11111111;
        n[7:4][5:2] = 4w0;
        debug.write(1, n);
        standard_meta.egress_spec = 0;
    }
}

control Eg(inout Headers hdrs,
               inout Metadata meta,
               inout standard_metadata_t standard_meta) {

    apply {
    }
}

// deparser section
control DP(packet_out b, in Headers p) {
    apply {
        b.emit(p.ethernet);
        b.emit(p.ip);
    }
}

// Fillers
control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {}
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {}
}

// Instantiate the top-level V1 Model package.
V1Switch(P(),
         Verify(),
         Ing(),
         Eg(),
         Compute(),
         DP()) main;
