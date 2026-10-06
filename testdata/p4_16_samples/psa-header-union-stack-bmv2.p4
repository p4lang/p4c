/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <bmv2/psa.p4>

header H1 {
    bit<8> a;
}

header H2 {
    bit<16> b;
}

header_union HU {
    H1 x;
    H2 y;
}

struct headers_t {
    HU[2] hus;
}

struct metadata_t {}
struct empty_t {}

parser IngressParserImpl(packet_in pkt,
                         out headers_t hdr,
                         inout metadata_t meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_t resubmit_meta,
                         in empty_t recirculate_meta) {
    state start {
        pkt.extract(hdr.hus[0].x);
        pkt.extract(hdr.hus[1].y);
        transition accept;
    }
}

control IngressImpl(inout headers_t hdr,
                    inout metadata_t meta,
                    in psa_ingress_input_metadata_t istd,
                    inout psa_ingress_output_metadata_t ostd) {
    apply {
        hdr.hus[0].x.a = hdr.hus[0].x.a + 1;
        hdr.hus[1].y.b = hdr.hus[1].y.b + 1;
        ostd.egress_port = (PortId_t) 1;
    }
}

control IngressDeparserImpl(packet_out pkt,
                            out empty_t clone_i2e_meta,
                            out empty_t resubmit_meta,
                            out empty_t normal_meta,
                            inout headers_t hdr,
                            in metadata_t meta,
                            in psa_ingress_output_metadata_t istd) {
    apply {
        pkt.emit(hdr.hus);
    }
}

parser EgressParserImpl(packet_in pkt,
                        out headers_t hdr,
                        inout metadata_t meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_t normal_meta,
                        in empty_t clone_i2e_meta,
                        in empty_t clone_e2e_meta) {
    state start {
        pkt.extract(hdr.hus[0].x);
        pkt.extract(hdr.hus[1].y);
        transition accept;
    }
}

control EgressImpl(inout headers_t hdr,
                   inout metadata_t meta,
                   in psa_egress_input_metadata_t istd,
                   inout psa_egress_output_metadata_t ostd) {
    apply {}
}

control EgressDeparserImpl(packet_out pkt,
                           out empty_t clone_e2e_meta,
                           out empty_t recirculate_meta,
                           inout headers_t hdr,
                           in metadata_t meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        pkt.emit(hdr.hus);
    }
}

IngressPipeline(IngressParserImpl(), IngressImpl(), IngressDeparserImpl()) ip;
EgressPipeline(EgressParserImpl(), EgressImpl(), EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
