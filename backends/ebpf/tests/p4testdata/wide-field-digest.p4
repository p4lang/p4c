/*
Copyright 2023-present Orange

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
#include "common_headers.p4"

struct headers {
    ethernet_t ethernet;
    ipv6_t     ipv6;
}

struct digest_t {
    bit<128> srcAddr;
    bit<16>  info;
}

parser IngressParserImpl(packet_in buffer,
                         out headers parsed_hdr,
                         inout empty_t meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_t resubmit_meta,
                         in empty_t recirculate_meta)
{
    state start {
        buffer.extract(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            0x86DD: parse_ipv6;
            default: accept;
        }
    }

    state parse_ipv6 {
        buffer.extract(parsed_hdr.ipv6);
        transition accept;
    }
}

control ingress(inout headers hdr,
                inout empty_t meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    apply {
        send_to_port(ostd, (PortId_t) PORT0);
    }
}

control IngressDeparserImpl(packet_out packet,
                            out empty_t clone_i2e_meta,
                            out empty_t resubmit_meta,
                            out empty_t normal_meta,
                            inout headers hdr,
                            in empty_t meta,
                            in psa_ingress_output_metadata_t istd)
{
    Digest<digest_t>() d;
    apply {
        d.pack({ hdr.ipv6.srcAddr, 1023 });

        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv6);
    }
}

parser EgressParserImpl(packet_in buffer,
                        out headers parsed_hdr,
                        inout empty_t meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_t normal_meta,
                        in empty_t clone_i2e_meta,
                        in empty_t clone_e2e_meta)
{
    state start {
        transition accept;
    }
}

control egress(inout headers hdr,
               inout empty_t meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply {}
}

control EgressDeparserImpl(packet_out buffer,
                           out empty_t clone_e2e_meta,
                           out empty_t recirculate_meta,
                           inout headers hdr,
                           in empty_t meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd)
{
    apply {}
}

IngressPipeline(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;
EgressPipeline(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;
PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
