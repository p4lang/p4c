/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <psa.p4>

struct empty_t {
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
    bit<32> element0;
    bit<32> element1;
    bit<32> element2;
    bit<32> element3;
}

parser IngressParserImpl(
    packet_in packet,
    out headers_t headers,
    inout metadata_t user_meta,
    in psa_ingress_parser_input_metadata_t istd,
    in empty_t resubmit_meta,
    in empty_t recirculate_meta
) {
    state start {
        packet.extract(headers.ethernet);
        transition accept;
    }
}

parser EgressParserImpl(
    packet_in packet,
    out headers_t headers,
    inout metadata_t user_meta,
    in psa_egress_parser_input_metadata_t istd,
    in empty_t normal_meta,
    in empty_t clone_i2e_meta,
    in empty_t clone_e2e_meta
) {
    state start {
        transition accept;
    }
}

control RegisterIngress(
    inout headers_t headers,
    inout metadata_t user_meta,
    in psa_ingress_input_metadata_t istd,
    inout psa_ingress_output_metadata_t ostd
) {
    Register<bit<32>, PortId_t>(4) register_array;

    apply {
        user_meta.element0 = register_array.read((PortId_t)0);
        user_meta.element1 = register_array.read((PortId_t)1);
        user_meta.element2 = register_array.read((PortId_t)2);
        user_meta.element3 = register_array.read((PortId_t)3);

        register_array.write((PortId_t)0, user_meta.element0 + 1);
        register_array.write((PortId_t)1, user_meta.element1 + 1);
        register_array.write((PortId_t)2, user_meta.element2 + 1);
        register_array.write((PortId_t)3, user_meta.element3 + 1);
    }
}

control RegisterEgress(
    inout headers_t headers,
    inout metadata_t user_meta,
    in psa_egress_input_metadata_t istd,
    inout psa_egress_output_metadata_t ostd
) {
    apply {
    }
}

control RegisterIngressDeparser(
    packet_out packet,
    out empty_t clone_i2e_meta,
    out empty_t resubmit_meta,
    out empty_t normal_meta,
    inout headers_t headers,
    in metadata_t user_meta,
    in psa_ingress_output_metadata_t istd
) {
    apply {
        packet.emit(headers.ethernet);
    }
}

control RegisterEgressDeparser(
    packet_out packet,
    out empty_t clone_e2e_meta,
    out empty_t recirculate_meta,
    inout headers_t headers,
    in metadata_t user_meta,
    in psa_egress_output_metadata_t istd,
    in psa_egress_deparser_input_metadata_t edstd
) {
    apply {
    }
}

IngressPipeline(
    IngressParserImpl(),
    RegisterIngress(),
    RegisterIngressDeparser()
) ingress;
EgressPipeline(
    EgressParserImpl(),
    RegisterEgress(),
    RegisterEgressDeparser()
) egress;

PSA_Switch(
    ingress,
    PacketReplicationEngine(),
    egress,
    BufferingQueueingEngine()
) main;
