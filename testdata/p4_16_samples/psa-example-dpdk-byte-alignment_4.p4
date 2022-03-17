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
#include "bmv2/psa.p4"


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

struct empty_metadata_t {
}

struct fwd_metadata_t {
    bit<32> old_srcAddr;
}

struct metadata {
    fwd_metadata_t fwd_metadata;
}

struct headers {
    ethernet_t       ethernet;
    ipv4_t           ipv4;
    tcp_t            tcp;
}


// Define additional error values, one of them for packets with
// incorrect IPv4 header checksums.
error {
    UnhandledIPv4Options,
    BadIPv4HeaderChecksum
}


parser IngressParserImpl(packet_in buffer,
                         out headers parsed_hdr,
                         inout metadata user_meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta)
{
    state start {
        buffer.extract(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract(parsed_hdr.ipv4);
        //verify(parsed_hdr.ipv4.ihl == 5, error.UnhandledIPv4Options);
        transition select(parsed_hdr.ipv4.protocol) {
            6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        buffer.extract(parsed_hdr.tcp);
        transition accept;
    }
}

// BEGIN:Incremental_Checksum_Table
control ingress(inout headers hdr,
                inout metadata user_meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd) {
    action drop() {
      ingress_drop(ostd);
    }
    action forward(PortId_t port, bit<32> srcAddr) {
      user_meta.fwd_metadata.old_srcAddr = hdr.ipv4.srcAddr;
      hdr.ipv4.srcAddr = srcAddr;
      send_to_port(ostd, port);
    }
    table route {
        key = { hdr.ipv4.dstAddr : lpm; }
        actions = {
          forward;
          drop;
        }
    }
    apply {
        if(hdr.ipv4.isValid()) {
          route.apply();
        }
    }
}
// END:Incremental_Checksum_Table

parser EgressParserImpl(packet_in buffer,
                        out headers parsed_hdr,
                        inout metadata user_meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_metadata_t normal_meta,
                        in empty_metadata_t clone_i2e_meta,
                        in empty_metadata_t clone_e2e_meta)
{
    state start {
        transition accept;
    }
}

control egress(inout headers hdr,
               inout metadata user_meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply { }
}

control IngressDeparserImpl(packet_out packet,
                            out empty_metadata_t clone_i2e_meta,
                            out empty_metadata_t resubmit_meta,
                            out empty_metadata_t normal_meta,
                            inout headers hdr,
                            in metadata user_meta,
                            in psa_ingress_output_metadata_t istd)
{
	InternetChecksum() ck;
    apply {
	 ck.clear();
        ck.add({
            /* 16-bit word  0   */ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv,
            /* 16-bit word  1   */ hdr.ipv4.totalLen,
            /* 16-bit word  2   */ hdr.ipv4.identification,
            /* 16-bit word  3   */ hdr.ipv4.flags, hdr.ipv4.fragOffset,
            /* 16-bit word  4   */ hdr.ipv4.ttl, hdr.ipv4.protocol,
            /* 16-bit word  5 skip hdr.ipv4.hdrChecksum, */
            /* 16-bit words 6-7 */ hdr.ipv4.srcAddr,
            /* 16-bit words 8-9 */ hdr.ipv4.dstAddr
            });
        hdr.ipv4.hdrChecksum = ck.get();
        // Update TCP checksum
        // This clear() call is necessary for correct behavior, since
        // the same instance 'ck' is reused from above for the same
        // packet.  If a second InternetChecksum instance other than
        // 'ck' were used below instead, this clear() call would be
        // unnecessary.
        ck.clear();
        // Subtract the original TCP checksum
        ck.subtract({hdr.tcp.checksum});
        // Subtract the effect of the original IPv4 source address,
        // which is part of the TCP 'pseudo-header' for the purposes
        // of TCP checksum calculation (see RFC 793), then add the
        // effect of the new IPv4 source address.
        ck.subtract({user_meta.fwd_metadata.old_srcAddr});
        ck.add({hdr.ipv4.srcAddr});
        hdr.tcp.checksum = ck.get();
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
    }
}

// BEGIN:Incremental_Checksum_Example
control EgressDeparserImpl(packet_out packet,
                           out empty_metadata_t clone_e2e_meta,
                           out empty_metadata_t recirculate_meta,
                           inout headers hdr,
                           in metadata user_meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd)
{
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
