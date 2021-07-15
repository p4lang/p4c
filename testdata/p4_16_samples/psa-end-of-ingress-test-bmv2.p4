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
#include "bmv2/psa.p4"


typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header output_data_t {
    bit<32> word0;
    bit<32> word1;
    bit<32> word2;
    bit<32> word3;
}

struct empty_metadata_t {
}

struct metadata_t {
}

struct headers_t {
    ethernet_t       ethernet;
    output_data_t    output_data;
}

control packet_path_to_int (in PSA_PacketPath_t packet_path,
                            out bit<32> ret)
{
    apply {
        // Unconditionally assign a value of 8 to ret, because if all
        // assignments to ret are conditional, p4c gives warnings
        // about ret possibly being uninitialized.
        ret = 8;
        if (packet_path == PSA_PacketPath_t.NORMAL) {
            ret = 1;
        } else if (packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
            ret = 2;
        } else if (packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
            ret = 3;
        } else if (packet_path == PSA_PacketPath_t.CLONE_I2E) {
            ret = 4;
        } else if (packet_path == PSA_PacketPath_t.CLONE_E2E) {
            ret = 5;
        } else if (packet_path == PSA_PacketPath_t.RESUBMIT) {
            ret = 6;
        } else if (packet_path == PSA_PacketPath_t.RECIRCULATE) {
            ret = 7;
        }
        // ret should still be 8 if packet_path is not any of those
        // enum values, which according to the P4_16 specification,
        // could happen if packet_path were uninitialized.
    }
}

parser IngressParserImpl(packet_in pkt,
                         out headers_t hdr,
                         inout metadata_t user_meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta)
{
    state start {
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.output_data);
        transition accept;
    }
}

control cIngress(inout headers_t hdr,
                 inout metadata_t user_meta,
                 in    psa_ingress_input_metadata_t  istd,
                 inout psa_ingress_output_metadata_t ostd)
{
    apply {
        ostd.drop = (bool) hdr.ethernet.dstAddr[40:40];
        if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
            // If packet was resubmitted, do not resubmit it again
        } else {
            ostd.resubmit = (bool) hdr.ethernet.dstAddr[32:32];
        }
        ostd.multicast_group = (MulticastGroup_t)
            ((MulticastGroupUint_t) hdr.ethernet.dstAddr[31:16]);
        ostd.egress_port = (PortId_t) ((PortIdUint_t) hdr.ethernet.dstAddr[15:0]);
        // Recording this value helps to determine in the output
        // packets whether the packet was resubmitted, or not.
        packet_path_to_int.apply(istd.packet_path, hdr.output_data.word0);
    }
}

parser EgressParserImpl(packet_in pkt,
                        out headers_t hdr,
                        inout metadata_t user_meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_metadata_t normal_meta,
                        in empty_metadata_t clone_i2e_meta,
                        in empty_metadata_t clone_e2e_meta)
{
    state start {
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.output_data);
        transition accept;
    }
}

control cEgress(inout headers_t hdr,
                inout metadata_t user_meta,
                in    psa_egress_input_metadata_t  istd,
                inout psa_egress_output_metadata_t ostd)
{
    Register<bit<16>, bit<8>>(256) egress_pkt_seen;

    apply {
        bit<8> idx = hdr.ethernet.etherType[7:0];
        bit<16> cur_count = egress_pkt_seen.read(idx);
        if (hdr.ethernet.etherType[15:8] == 0xc0) {
            // 'command packet' to read register
            hdr.output_data.word0 = (bit<32>) cur_count;
        } else if (hdr.ethernet.etherType[15:8] == 0xc1) {
            // 'command packet' to write register
            bit<16> write_data = hdr.ethernet.srcAddr[15:0];
            egress_pkt_seen.write(idx, write_data);
        } else {
            // 'data packet' that should be recorded that it went
            // through this code path.  By remembering a packet got
            // here, and using 'command packets' to read/write the
            // 'got here' values before and after sending 'data
            // packets', we can determine from the output packet
            // contents alone whether a packet was dropped during
            // ingress and never did egress processing, or did do
            // egress processing.

            // I tried using a counter instead, but I could not see a
            // way given the existing STF test file infrastructure to
            // deterministically wait until all data packets were
            // processed before sending in the command packet that
            // reads this value, so I would get packet count values
            // ranging between 8 and 12, instead of the deterministic
            // 12 I was hoping for.  Changing it to a flag indicating
            // that at least one packet was seen gives much more
            // repeatable results.
            if (hdr.ethernet.etherType < 256) {
                egress_pkt_seen.write(idx, 1);
            }
            hdr.output_data.word1 = (bit<32>) istd.egress_port;
            hdr.output_data.word2 = (bit<32>) ((EgressInstanceUint_t) istd.instance);
            packet_path_to_int.apply(istd.packet_path, hdr.output_data.word3);
        }
    }
}

control CommonDeparserImpl(packet_out packet,
                           inout headers_t hdr)
{
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.output_data);
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
