/*
Copyright 2017 Cisco Systems, Inc.

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
#include <v1model.p4>

typedef bit<48>  EthernetAddress;
typedef bit<32>  IPv4Address;

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

// IPv4 header _with_ options
header ipv4_t {
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
    // This is significantly shorter than the IPv4 specification
    // allows for options.  It is written this way specifically to
    // make it easier to write an STF test case that exhibits the bug
    // described in p4c issue #1025.
    varbit<32>   options;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header IPv4_up_to_ihl_only_h {
    bit<4>       version;
    bit<4>       ihl;
}

struct headers {
    ethernet_t    ethernet;
    ipv4_t        ipv4;
    tcp_t         tcp;
}

struct metadata {
}

error {
    IPv4HeaderTooShort,
    IPv4IncorrectVersion,
    IPv4ChecksumError
}

parser parserI(packet_in pkt,
               out headers hdr,
               inout metadata meta,
               inout standard_metadata_t stdmeta)
{
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        ////////////////////////////////////////////////////////////
        // WARNING WARNING WARNING
        ////////////////////////////////////////////////////////////
        // This is NOT the correct way to extract IPv4
        // options according the the standards.  This is a hacked-up
        // example that allows us to make STF tests cases for issue
        // #1025 that can pass/fail based upon bmv2 detecting a
        // HeaderTooShort parser error at exactly the right number of
        // bytes of variable length header, even detecting a bug that
        // was off by 1 byte.
        pkt.extract(hdr.ipv4,
                    (bit<32>)
                    (8 *
                     (bit<9>) (pkt.lookahead<IPv4_up_to_ihl_only_h >().ihl)));
        ////////////////////////////////////////////////////////////
        // WARNING WARNING WARNING
        ////////////////////////////////////////////////////////////
        transition select (hdr.ipv4.protocol) {
            6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }
}

control cIngress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t stdmeta)
{
    bit<1> eth_valid;
    bit<1> ipv4_valid;
    bit<1> tcp_valid;
    apply {
        eth_valid = (bit<1>) hdr.ethernet.isValid();
        ipv4_valid = (bit<1>) hdr.ipv4.isValid();
        tcp_valid = (bit<1>) hdr.tcp.isValid();
        hdr.ethernet.dstAddr = (24w0 ++
                                7w0 ++ eth_valid ++
                                7w0 ++ ipv4_valid ++
                                7w0 ++ tcp_valid);
    }
}

control cEgress(inout headers hdr,
                inout metadata meta,
                inout standard_metadata_t stdmeta)
{
    apply { }
}

control verifyChecksum(inout headers hdr,
                       inout metadata meta)
{
    apply { }
}

control updateChecksum(inout headers hdr,
                       inout metadata meta)
{
    apply { }
}

control DeparserI(packet_out packet,
                  in headers hdr)
{
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
    }
}

V1Switch(parserI(),
         verifyChecksum(),
         cIngress(),
         cEgress(),
         updateChecksum(),
         DeparserI()) main;
