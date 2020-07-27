/*
 * Copyright 2020 MNK Labs & Consulting, LLC.
 *
 * https://mnkcg.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Jointly developed with Andy Fingerhut (ex-Cisco and Barefoot/Intel).
 *
 * This program does not support IPv6.  However, IPv6 also uses
 * a pseudo header for upper-layer protocols (e.g., TCP and UDP).
 * More details for IPv6 checksum is available at the link below.
 *
 * https://tools.ietf.org/html/rfc8200#page-27
 *
 * The program does not support UDP options because none have been specified
 * in a RFC.
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
    varbit<320>  options;
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
    bit<16> checksum; // Includes Pseudo Hdr + TCP segment (hdr + payload)
    bit<16> urgentPtr;
    varbit<320>  options;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

header IPv4_up_to_ihl_only_h {
    bit<4>       version;
    bit<4>       ihl;
}

header tcp_upto_data_offset_only_h {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    // dataOffset in TCP hdr uses 4 bits but needs padding.
    // If 4 bits are used for it, p4c-bm2-ss complains the header
    // is not a multiple of 8 bits.
    bit<4>  dataOffset;
    bit<4>  dontCare;
}

struct headers {
    ethernet_t           ethernet;
    ipv4_t               ipv4;
    tcp_t                tcp;
    udp_t                udp;
}

struct mystruct1_t {
    bit<4>  a;
    bit<4>  b;
}

struct metadata {
    mystruct1_t mystruct1;
    bit<16>     l4Len; // includes TCP hdr len + TCP payload len in bytes.
}

typedef tuple<
    bit<4>,
    bit<4>,
    bit<8>,
    varbit<56>
    > myTuple1;

// Declare user-defined errors that may be signaled during parsing
error {
    IPv4HeaderTooShort,
    TCPHeaderTooShort,
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
        // The 4-bit IHL field of the IPv4 base header is the number
        // of 32-bit words in the entire IPv4 header.  It is an error
        // for it to be less than 5.  There are only IPv4 options
        // present if the value is at least 6.  The length of the IPv4
        // options alone, without the 20-byte base header, is thus ((4
        // * ihl) - 20) bytes, or 8 times that many bits.
        pkt.extract(hdr.ipv4,
                    (bit<32>)
                    (8 *
                     (4 * (bit<9>) (pkt.lookahead<IPv4_up_to_ihl_only_h>().ihl)
                      - 20)));
        verify(hdr.ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hdr.ipv4.ihl >= 4w5, error.IPv4HeaderTooShort);
        meta.l4Len = hdr.ipv4.totalLen - (bit<16>)(hdr.ipv4.ihl)*4;
        transition select (hdr.ipv4.protocol) {
            6: parse_tcp;
            17: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        // The 4-bit dataOffset field of the TCP base header is the number
        // of 32-bit words in the entire TCP header.  It is an error
        // for it to be less than 5.  There are only TCP options
        // present if the value is at least 6.  The length of the TCP
        // options alone, without the 20-byte base header, is thus ((4
        // * dataOffset) - 20) bytes, or 8 times that many bits.
        pkt.extract(hdr.tcp,
                    (bit<32>)
                    (8 *
                     (4 * (bit<9>) (pkt.lookahead<tcp_upto_data_offset_only_h>().dataOffset)
                      - 20)));
        verify(hdr.tcp.dataOffset >= 4w5, error.TCPHeaderTooShort);
        transition accept;
    }
    state parse_udp {
        pkt.extract(hdr.udp);
        transition accept;
    }

}

control cIngress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t stdmeta)
{
    action foot() {
        hdr.tcp.srcPort = hdr.tcp.srcPort + 1;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
        hdr.ipv4.dstAddr = hdr.ipv4.dstAddr + 4;
    }
    action foou() {
        hdr.udp.srcPort = hdr.udp.srcPort + 1;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
        hdr.ipv4.dstAddr = hdr.ipv4.dstAddr + 4;
    }
    table guh {
        key = {
            hdr.tcp.dstPort : exact;
        }
        actions = { foot; NoAction; }
        const default_action = NoAction;

        const entries = {
            (16w80) : foot();
        }
    }
    table huh {
        key = {
            hdr.udp.dstPort : exact;
        }
        actions = { foou; NoAction; }
        const default_action = NoAction;

        const entries = {
            (16w80) : foou();
        }
    }
    apply {
        if (hdr.tcp.isValid()) {
            guh.apply();
        }
        if (hdr.udp.isValid()) {
            huh.apply();
        }
    }
}

control cEgress(inout headers hdr,
                inout metadata meta,
                inout standard_metadata_t stdmeta)
{
    apply {
    }
}

control vc(inout headers hdr,
           inout metadata meta)
{
    apply {
        verify_checksum(true,
            { hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.diffserv,
                hdr.ipv4.totalLen,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.fragOffset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr,
                hdr.ipv4.options
            },
            hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum_with_payload(hdr.tcp.isValid(),
            { hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr,
                8w0,
                hdr.ipv4.protocol,
                meta.l4Len,
                hdr.tcp.srcPort,
                hdr.tcp.dstPort,
                hdr.tcp.seqNo,
                hdr.tcp.ackNo,
                hdr.tcp.dataOffset,
                hdr.tcp.res,
                hdr.tcp.ecn,
                hdr.tcp.ctrl,
                hdr.tcp.window,
                hdr.tcp.urgentPtr,
                hdr.tcp.options
            },
            hdr.tcp.checksum, HashAlgorithm.csum16);
        verify_checksum_with_payload(hdr.udp.isValid(),
            { hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr,
                8w0,
                hdr.ipv4.protocol,
                meta.l4Len,
                hdr.udp.srcPort,
                hdr.udp.dstPort,
                hdr.udp.length_
            },
            hdr.udp.checksum, HashAlgorithm.csum16);
    }
}

control uc(inout headers hdr,
           inout metadata meta)
{
    apply {
        update_checksum(true,
            { hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.diffserv,
                hdr.ipv4.totalLen,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.fragOffset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr,
                hdr.ipv4.options
            },
            hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);

        update_checksum_with_payload(hdr.tcp.isValid(),
            { hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr,
                8w0,
                hdr.ipv4.protocol,
                meta.l4Len,
                hdr.tcp.srcPort,
                hdr.tcp.dstPort,
                hdr.tcp.seqNo,
                hdr.tcp.ackNo,
                hdr.tcp.dataOffset,
                hdr.tcp.res,
                hdr.tcp.ecn,
                hdr.tcp.ctrl,
                hdr.tcp.window,
                hdr.tcp.urgentPtr,
                hdr.tcp.options
            },
            hdr.tcp.checksum, HashAlgorithm.csum16);
        update_checksum_with_payload(hdr.udp.isValid(),
            { hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr,
                8w0,
                hdr.ipv4.protocol,
                meta.l4Len,
                hdr.udp.srcPort,
                hdr.udp.dstPort,
                hdr.udp.length_
            },
            hdr.udp.checksum, HashAlgorithm.csum16);
    }
}

control DeparserI(packet_out packet,
                  in headers hdr)
{
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
        packet.emit(hdr.udp);
    }
}

V1Switch<headers, metadata>(parserI(),
                            vc(),
                            cIngress(),
                            cEgress(),
                            uc(),
                            DeparserI()) main;
