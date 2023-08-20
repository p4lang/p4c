/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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

header crc_t {
    bit<8>  f1;
    bit<32> f3;
    bit<32> f4;

    bit<8> crc;
}

header clone_i2e_metadata_t {
}

struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    crc_t     crc;
}

parser IngressParserImpl(
    packet_in buffer,
    out headers parsed_hdr,
    inout metadata user_meta,
    in psa_ingress_parser_input_metadata_t istd,
    in empty_metadata_t resubmit_meta,
    in empty_metadata_t recirculate_meta)
{
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        buffer.extract(parsed_hdr.ethernet);
        transition parse_crc;
    }
    state parse_crc {
        buffer.extract(parsed_hdr.crc);
        transition accept;
    }
}


control ingress(inout headers hdr,
                inout metadata user_meta,
                in  psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    Hash<bit<16>>(PSA_HashAlgorithm_t.CRC16) h;

    apply {
        send_to_port(ostd, (PortId_t) PORT1);

        hdr.crc.crc = (bit<8>) h.get_hash(16w50, {hdr.crc.f1, hdr.crc.f3, hdr.crc.f4}, 16w200);
    }
}

control IngressDeparserImpl(
    packet_out packet,
    out clone_i2e_metadata_t clone_i2e_meta,
    out empty_metadata_t resubmit_meta,
    out metadata normal_meta,
    inout headers parsed_hdr,
    in metadata meta,
    in psa_ingress_output_metadata_t istd)
{
    apply {
        packet.emit(parsed_hdr.ethernet);
        packet.emit(parsed_hdr.crc);
    }
}

parser EgressParserImpl(
    packet_in buffer,
    out headers parsed_hdr,
    inout metadata user_meta,
    in psa_egress_parser_input_metadata_t istd,
    in metadata normal_meta,
    in clone_i2e_metadata_t clone_i2e_meta,
    in empty_metadata_t clone_e2e_meta)
{
    state start {
        transition accept;
    }
}

control egress(inout headers hdr,
               inout metadata user_meta,
               in  psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply {}
}

control EgressDeparserImpl(
    packet_out packet,
    out empty_metadata_t clone_e2e_meta,
    out empty_metadata_t recirculate_meta,
    inout headers parsed_hdr,
    in metadata meta,
    in psa_egress_output_metadata_t istd,
    in psa_egress_deparser_input_metadata_t edstd)
{
    apply {}
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
