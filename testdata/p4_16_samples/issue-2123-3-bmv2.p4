/*
 * Copyright 2020, MNK Labs & Consulting
 * http://mnkcg.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header h0_t {
    bit<8>  f0;
}

header h1_t {
    bit<8>  f1;
}

header h2_t {
    bit<8>  f2;
}

header h3_t {
    bit<8>  f3;
}

header h4_t {
    bit<8>  f4;
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    h0_t       h0;
    h1_t       h1;
    h2_t       h2;
    h3_t       h3;
    h4_t       h4;
}

parser ParserImpl(
    packet_in packet,
    out headers hdr,
    inout metadata meta,
    inout standard_metadata_t standard_metadata)
{
    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.srcAddr[7:0], hdr.ethernet.etherType) {
            (0x61 .. 0x67, 0x0800 .. 0x0806): parse_h0;
            (0x61 .. 0x67, 0x0901 .. 0x0902): parse_h1;
            (0x77 .. 0x7b, 0x0801 .. 0x0806): parse_h2;
            (0x77 .. 0x7b, 0x0a00 .. 0x0aaa): parse_h3;
            (           _, 0x0a00 .. 0x0aaa): parse_h4;
            default: accept;
        }
    }
    state parse_h0 {
        packet.extract(hdr.h0);
        transition accept;
    }
    state parse_h1 {
        packet.extract(hdr.h1);
        transition accept;
    }
    state parse_h2 {
        packet.extract(hdr.h2);
        transition accept;
    }
    state parse_h3 {
        packet.extract(hdr.h3);
        transition accept;
    }
    state parse_h4 {
        packet.extract(hdr.h4);
        transition accept;
    }
}

control ingress(
    inout headers hdr,
    inout metadata meta,
    inout standard_metadata_t standard_metadata)
{
    apply {
        // Overwrite some bits of one of the header fields so that in
        // the STF test we can match on the output packet contents and
        // know which case was taken in the select expression in the
        // parser.
        hdr.ethernet.dstAddr[44:44] = hdr.h4.isValid() ? 1w1 : 0;
        hdr.ethernet.dstAddr[43:43] = hdr.h3.isValid() ? 1w1 : 0;
        hdr.ethernet.dstAddr[42:42] = hdr.h2.isValid() ? 1w1 : 0;
        hdr.ethernet.dstAddr[41:41] = hdr.h1.isValid() ? 1w1 : 0;
        hdr.ethernet.dstAddr[40:40] = hdr.h0.isValid() ? 1w1 : 0;

        standard_metadata.egress_spec = 3;
    }
}

control egress(
    inout headers hdr,
    inout metadata meta,
    inout standard_metadata_t standard_metadata)
{
    apply {
    }
}

control DeparserImpl(
    packet_out packet,
    in headers hdr)
{
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.h0);
        packet.emit(hdr.h1);
        packet.emit(hdr.h2);
        packet.emit(hdr.h3);
        packet.emit(hdr.h4);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}

V1Switch(
    ParserImpl(),
    verifyChecksum(),
    ingress(),
    egress(),
    computeChecksum(),
    DeparserImpl())
main;
