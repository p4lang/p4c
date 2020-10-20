/*
Copyright 2020 Cornell University

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
#include "psa.p4"


typedef bit<48>  EthernetAddress;
typedef bit<32>  IPv4Address;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

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
}

struct empty_metadata_t {
}

struct metadata_t {
}

struct headers_t {
    ethernet_t       ethernet;
    ipv4_t           ipv4;
}

parser IngressParserImpl(packet_in pkt,
                         out headers_t hdr,
                         inout metadata_t user_meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta)
{
    // TODO InternetChecksum cannot be used out of control block yet.
    // This need to be fixed.
    // InternetChecksum() ck1;

    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            // default: accept
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);

        // ck1.clear();

        transition accept;
    }
}

control cIngress(inout headers_t hdr,
                 inout metadata_t user_meta,
                 in    psa_ingress_input_metadata_t  istd,
                 inout psa_ingress_output_metadata_t ostd)
{
    InternetChecksum() ck;
    bit<16> csum;
    bit<16> ck_state;
    apply {
        // TODO internetChecksum cannot take a list of fields yet.
        // If received a list of fields, internetChecksum should
        // concatenate the fields in the list.
        // ck.add({hdr.ethernet.dstAddr, hdr.ethernet.srcAddr});

        hdr.ethernet.dstAddr = 4;

        ck.clear();
        bit<16> test0;
        test0 = ck.get();
        bit<16> test1;
        test1 = ck.get_state();

        // test0 should be bit<16> all 1s, which is 65535
        if (test0 != 65535) {
            hdr.ethernet.dstAddr = hdr.ethernet.dstAddr + 1;
        }

        // test1 should be bit<16> all 0s, which is 0
        if (test1 != 0) {
            hdr.ethernet.dstAddr = hdr.ethernet.dstAddr + 1;
        }

        ck.add(hdr.ethernet.dstAddr);
        ck.subtract(hdr.ethernet.dstAddr);
        ck.add(hdr.ethernet.dstAddr);
        csum = ck.get();

        // ck's state should be 9, thus csum should be 65535 - 9
        if (csum == 65526) {
            hdr.ethernet.dstAddr = hdr.ethernet.dstAddr + 1;
        }

        ck.set_state(9);
        ck_state = ck.get_state();

        if (ck_state != 9) {
            hdr.ethernet.dstAddr = hdr.ethernet.dstAddr + 1;
        }

        send_to_port(ostd, (PortId_t) (PortIdUint_t) hdr.ethernet.dstAddr);
    }
}

parser EgressParserImpl(packet_in buffer,
                        out headers_t hdr,
                        inout metadata_t user_meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_metadata_t normal_meta,
                        in empty_metadata_t clone_i2e_meta,
                        in empty_metadata_t clone_e2e_meta)
{
    state start {
        buffer.extract(hdr.ethernet);
        transition accept;
    }
}

control cEgress(inout headers_t hdr,
                inout metadata_t user_meta,
                in    psa_egress_input_metadata_t  istd,
                inout psa_egress_output_metadata_t ostd)
{
    apply { }
}

control CommonDeparserImpl(packet_out packet,
                           inout headers_t hdr)
{
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

control IngressDeparserImpl(packet_out buffer,
                            out empty_metadata_t clone_i2e_meta,
                            out empty_metadata_t resubmit_meta,
                            out empty_metadata_t normal_meta,
                            inout headers_t hdr,
                            in metadata_t meta,
                            in psa_ingress_output_metadata_t istd)
{
    CommonDeparserImpl() cp;
    apply {
        cp.apply(buffer, hdr);
    }
}

control EgressDeparserImpl(packet_out buffer,
                           out empty_metadata_t clone_e2e_meta,
                           out empty_metadata_t recirculate_meta,
                           inout headers_t hdr,
                           in metadata_t meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd)
{
    CommonDeparserImpl() cp;
    apply {
        cp.apply(buffer, hdr);
    }
}

IngressPipeline(IngressParserImpl(),
                cIngress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               cEgress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
