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

bit<8> foofunc(in bit<8> i,   // line 2 - shadows line 1 i, p4c warns
               out bit<8> out1,
               out bit<8> out2)
{
    bit<8> tmp = i;
    bit<8> i = i + 1;      // line 3 - shadows line 2 i, p4c warns
    {
        out1 = tmp;        // line 4
        bit<8> i = i + 2;  // line 5 - shadows line 3 i, p4c warns
        out2 = i;          // line 6
    }
    return i;              // line 7
}

parser fooparser(inout bit<8> i,   // line 10 - shadows line 1 i, p4c warns
                 out bit<8> out1,
                 out bit<8> out2,
                 out bit<8> out3,
                 out bit<8> out4)
{
    bit<8> tmp = i;           // line 11
    bit<8> i = i + 1;         // line 12 - shadows line 10 i, p4c warns
    state start {
        out1 = tmp;           // line 13
        out2 = i;             // line 14
        {
            bit<8> i = i + 2; // line 15 - shadows line 12 i, p4c warns
            out3 = i;         // line 16
        }
        out4 = i;             // line 17
        transition accept;
    }
}

parser parserImpl(packet_in packet,
                  out headers_t hdr,
                  inout metadata_t meta,
                  inout standard_metadata_t stdmeta)
{
    bit<8> i;                // line 20 - shadows line 1 i, p4c warns
    bit<8> out1;
    bit<8> out2;
    bit<8> out3;
    bit<8> out4;
    state start {
        packet.extract(hdr.eth);
        i = hdr.eth.srcAddr[15:8];
        fooparser.apply(i, out1, out2, out3, out4);
        log_msg("i={} out1={} out2={} out3={} out4={}",
            {i, out1, out2, out3, out4});
        transition accept;
    }
}

control fooctrl(inout bit<8> i, // line 30 - shadows line 1 i, p4c warns
                out bit<8> out1,
                out bit<8> out2,
                out bit<8> out3,
                out bit<8> out4)
{
    bit<8> tmp = i;             // line 31
    bit<8> i = i + 1;           // line 32 - shadows line 30 i, p4c warns
    apply {
        out1 = tmp;             // line 33
        out2 = i;               // line 34
        {
            bit<8> i = i + 2;   // line 35 - shadows line 32 i, p4c warns
            out3 = i;           // line 36
        }
        out4 = i;               // line 37
    }
}

control ingressImpl(inout headers_t hdr,
                    inout metadata_t meta,
                    inout standard_metadata_t stdmeta)
{
    bit<8> i;             // line 40 - shadows line 1 i, p4c warns
    bit<8> out1;
    bit<8> out2;
    bit<8> out3;
    bit<8> out4;
    apply {
        i = hdr.eth.srcAddr[7:0];
        fooctrl.apply(i, out1, out2, out3, out4);
        log_msg("i={} out1={} out2={} out3={} out4={}",
            {i, out1, out2, out3, out4});
        out3 = foofunc(i, out1, out2);
        log_msg("i={} out1={} out2={} out3={}",
            {i, out1, out2, out3});
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
