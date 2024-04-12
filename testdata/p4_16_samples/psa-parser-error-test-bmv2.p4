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
#include <psa.p4>


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

header ipv6_t {
    bit<4>       version;
    bit<8>       trafficClass;
    bit<20>      flowLabel;
    bit<16>      payloadLen;
    bit<8>       nextHdr;
    bit<8>       hopLimit;
    bit<128>     srcAddr;
    bit<128>     dstAddr;
}

header mpls_t {
    bit<20> label;
    bit<3>  tc;
    bit<1>  stack;
    bit<8>  ttl;
}

struct empty_metadata_t {
}

struct fwd_metadata_t {
}

struct metadata {
    fwd_metadata_t fwd_metadata;
}

struct headers {
    ethernet_t       ethernet;
    ipv4_t           ipv4;
    ipv6_t           ipv6;
    mpls_t           mpls;
}

control parser_error_to_int (in error err, out bit<16> ret)
{
    apply {
        // Unconditionally assign an initial value to ret, because if
        // all assignments to ret are conditional, p4c gives warnings
        // about ret possibly being uninitialized.
        ret = 8;
        if (err == error.NoError) {
            ret = 1;
        } else if (err == error.PacketTooShort) {
            ret = 2;
        } else if (err == error.NoMatch) {
            ret = 3;
        } else if (err == error.StackOutOfBounds) {
            ret = 4;
        } else if (err == error.HeaderTooShort) {
            ret = 5;
        } else if (err == error.ParserTimeout) {
            ret = 6;
        } else if (err == error.ParserInvalidArgument) {
            ret = 7;
        }
        // ret should still be 8 if err is not any of those error
        // values, which according to the P4_16 specification, could
        // happen if err were uninitialized.
    }
}

parser IngressParserImpl(packet_in pkt,
                         out headers hdr,
                         inout metadata user_meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta)
{
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            0x8847: parse_mpls;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
    state parse_mpls {
        pkt.extract(hdr.mpls);
        transition accept;
    }
}

control ingress(inout headers hdr,
                inout metadata user_meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    apply {
        send_to_port(ostd, (PortId_t) (PortIdUint_t) hdr.ethernet.dstAddr);
        // Note that a more portable way to test for parser error
        // handling is to do a resubmit or ingress-to-egress clone
        // after detecting that a packet experienced a parser error,
        // because those operations preserve the packet as it was
        // before the parser found the error.  However, the PSA
        // implementation as of the date this test program was written
        // does not implement the preservation of user-defined
        // metadata with such packets, so there is not a good way to
        // pass along which error occurred in such packets.
        parser_error_to_int.apply(istd.parser_error, hdr.ethernet.dstAddr[47:32]);
        hdr.ethernet.dstAddr[31:0] = (bit<32>) (TimestampUint_t) istd.ingress_timestamp;
    }
}

parser EgressParserImpl(packet_in pkt,
                        out headers hdr,
                        inout metadata user_meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_metadata_t normal_meta,
                        in empty_metadata_t clone_i2e_meta,
                        in empty_metadata_t clone_e2e_meta)
{
    // Note: This is a very atypical P4 program, in that it parses
    // IPv4 headers but not IPv6 in the ingress parser, and vice versa
    // in the egress parser.  The only reason this is done is to make
    // it easy to provide input packets that have a parsing error
    // during ingress but not egress, and vice versa, so that we can
    // test whether both of the parsers correctly send out the parsing
    // error they encountered, independently of each other.
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x86dd: parse_ipv6;
            0x8847: parse_mpls;
            default: accept;
        }
    }
    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition accept;
    }
    state parse_mpls {
        pkt.extract(hdr.mpls);
        transition accept;
    }
}

control egress(inout headers hdr,
               inout metadata user_meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply {
        parser_error_to_int.apply(istd.parser_error, hdr.ethernet.srcAddr[47:32]);
        hdr.ethernet.srcAddr[31:0] = (bit<32>) (TimestampUint_t) istd.egress_timestamp;
    }
}

control IngressDeparserImpl(packet_out pkt,
                            out empty_metadata_t clone_i2e_meta,
                            out empty_metadata_t resubmit_meta,
                            out empty_metadata_t normal_meta,
                            inout headers hdr,
                            in metadata meta,
                            in psa_ingress_output_metadata_t istd)
{
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.mpls);
    }
}

control EgressDeparserImpl(packet_out pkt,
                           out empty_metadata_t clone_e2e_meta,
                           out empty_metadata_t recirculate_meta,
                           inout headers hdr,
                           in metadata meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd)
{
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv6);
        pkt.emit(hdr.mpls);
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
