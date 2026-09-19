/*
Copyright 2026 Andy Fingerhut

SPDX-License-Identifier: Apache-2.0
*/

#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t eth;
}

struct metadata_t {
}

const bit<8> i = 5;        // line 1 - p4c warns this is unused

bit<8> foofunc(in bit<8> i,  // line 2 - shadows line 1, p4c warns
               in bit<8> j)
{
    bit<8> i = 5;          // line 3 - p4c errors because shadows i in line 2
    return i + j;
}

parser parserImpl(packet_in packet,
                  out headers_t hdr,
                  inout metadata_t meta,
                  inout standard_metadata_t stdmeta)
{
    state start {
        packet.extract(hdr.eth);
        transition accept;
    }
}

control ingressImpl(inout headers_t hdr,
                    inout metadata_t meta,
                    inout standard_metadata_t stdmeta)
{
    bit<8> i;
    bit<8> j;
    bit<8> out1;
    apply {
        i = hdr.eth.srcAddr[7:0];
        j = hdr.eth.srcAddr[15:8];
        out1 = foofunc(i, j);
        log_msg("i={} j={} out1={}",
            {i, j, out1});
    }
}

control egressImpl(inout headers_t hdr,
                   inout metadata_t meta,
                   inout standard_metadata_t stdmeta)
{ apply { } }

control deparserImpl(packet_out packet, in headers_t hdr)
{ apply { } }

control verifyChecksum(inout headers_t hdr, inout metadata_t meta)
{ apply { } }

control updateChecksum(inout headers_t hdr, inout metadata_t meta)
{ apply { } }

V1Switch(parserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         deparserImpl()) main;
