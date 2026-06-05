/*
 * Copyright 2018 Cisco Systems, Inc.
 * SPDX-FileCopyrightText: 2018 Cisco Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct meta_t {
}

struct headers_t {
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet,
                  out headers_t hdr,
                  inout meta_t meta,
                  inout standard_metadata_t standard_metadata)
{
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers_t hdr,
                inout meta_t meta,
                inout standard_metadata_t standard_metadata)
{
    apply {
        bit<16> rand_int;

        // Compiler Bug: typechecker mutated program with this line
        // and latest version of compiler as of 2018-Sep-24:
        random<bit<16>>(rand_int, 0, 48*1024-1);

        // No error with this line:
        //random<bit<16>>(rand_int, 0, (bit<16>) 48*1024-1);

        if (rand_int < 32*1024) {
            mark_to_drop(standard_metadata);
        }
    }
}

control egress(inout headers_t hdr,
               inout meta_t meta,
               inout standard_metadata_t standard_metadata)
{
    apply { }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply { }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply { }
}

V1Switch(ParserImpl(),
         verifyChecksum(),
         ingress(),
         egress(),
         computeChecksum(),
         DeparserImpl()) main;
