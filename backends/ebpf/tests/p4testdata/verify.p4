/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <psa.p4>
#include "common_headers.p4"

struct metadata {
}

struct headers {
    ethernet_t       ethernet;
}

error {
    InvalidEtherType,
    InvalidEtherAddr,
    UnhandledIPv4Options // not used
}

parser IngressParserImpl(
    packet_in buffer,
    out headers parsed_hdr,
    inout metadata user_meta,
    in psa_ingress_parser_input_metadata_t istd,
    in empty_t resubmit_meta,
    in empty_t recirculate_meta)
{
    state start {
        buffer.extract(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            0xFF00: reject;
            default: verify_eth;
        }
    }
    state verify_eth {
        verify(parsed_hdr.ethernet.etherType == 0x0800, error.InvalidEtherType);
        transition accept;
    }
}


control ingress(inout headers hdr,
                inout metadata user_meta,
                in  psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    apply {
        if (istd.parser_error != error.NoError) {
            ostd.drop = true;
            return;
        }

        send_to_port(ostd, (PortId_t) PORT1);
    }
}

parser EgressParserImpl(
    packet_in buffer,
    out headers parsed_hdr,
    inout metadata user_meta,
    in psa_egress_parser_input_metadata_t istd,
    in metadata normal_meta,
    in empty_t clone_i2e_meta,
    in empty_t clone_e2e_meta)
{
    state start {
        buffer.extract(parsed_hdr.ethernet);
        verify(parsed_hdr.ethernet.srcAddr != 0, error.InvalidEtherAddr);
        transition accept;
    }
}

control egress(inout headers hdr,
               inout metadata user_meta,
               in  psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply {
        if (istd.parser_error != error.NoError) {
            ostd.drop = true;
            return;
        }

        ostd.drop = false;
    }
}

control IngressDeparserImpl(
    packet_out packet,
    out empty_t clone_i2e_meta,
    out empty_t resubmit_meta,
    out metadata normal_meta,
    inout headers hdr,
    in metadata meta,
    in psa_ingress_output_metadata_t istd)
{
    apply {
        packet.emit(hdr.ethernet);
    }
}

control EgressDeparserImpl(
    packet_out packet,
    out empty_t clone_e2e_meta,
    out empty_t recirculate_meta,
    inout headers hdr,
    in metadata meta,
    in psa_egress_output_metadata_t istd,
    in psa_egress_deparser_input_metadata_t edstd)
{
    apply {
        packet.emit(hdr.ethernet);
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
