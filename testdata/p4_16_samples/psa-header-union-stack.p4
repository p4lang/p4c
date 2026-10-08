/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <bmv2/psa.p4>

#include "spec-ex09.p4"

header empty_t {}
header parse_report_t {
    bit<8> parsed_count;
    bit<8> first_member;
}
struct metadata_t {
    bit<8> parsed_count;
    bit<8> first_member;
}
struct headers_t {
    parse_report_t report;
    Tcp_option_stack options;
}
struct TcpOptionSackLength {
    bit<8> kind;
    bit<8> length;
}

parser HeaderUnionIngressParser(
    packet_in packet,
    out headers_t hdr,
    inout metadata_t meta,
    in psa_ingress_parser_input_metadata_t istd,
    in empty_t resubmit,
    in empty_t recirculate) {
    state start {
        transition select(packet.lookahead<bit<8>>()) {
            8w0x00: parse_end;
            8w0x01: parse_nop;
            8w0x02: parse_ss;
            8w0x03: parse_s;
            8w0x05: parse_sack;
            default: accept;
        }
    }

    state parse_end {
        packet.extract(hdr.options.next.end);
        if (meta.parsed_count == 0) {
            meta.first_member = 0;
        }
        meta.parsed_count = meta.parsed_count + 1;
        transition accept;
    }

    state parse_nop {
        packet.extract(hdr.options.next.nop);
        if (meta.parsed_count == 0) {
            meta.first_member = 1;
        }
        meta.parsed_count = meta.parsed_count + 1;
        transition start;
    }

    state parse_ss {
        packet.extract(hdr.options.next.ss);
        if (meta.parsed_count == 0) {
            meta.first_member = 2;
        }
        meta.parsed_count = meta.parsed_count + 1;
        transition start;
    }

    state parse_s {
        packet.extract(hdr.options.next.s);
        if (meta.parsed_count == 0) {
            meta.first_member = 3;
        }
        meta.parsed_count = meta.parsed_count + 1;
        transition start;
    }

    state parse_sack {
        packet.extract(
            hdr.options.next.sack,
            (bit<32>)(8 * packet.lookahead<TcpOptionSackLength>().length - 16));
        if (meta.parsed_count == 0) {
            meta.first_member = 5;
        }
        meta.parsed_count = meta.parsed_count + 1;
        transition start;
    }
}

parser HeaderUnionEgressParser(
    packet_in packet,
    out headers_t hdr,
    inout metadata_t meta,
    in psa_egress_parser_input_metadata_t estd,
    in empty_t normal,
    in empty_t clone_i2e,
    in empty_t clone_e2e) {
    state start {
        packet.extract(hdr.report);
        transition parse_options;
    }

    state parse_options {
        transition select(packet.lookahead<bit<8>>()) {
            8w0x00: parse_end;
            8w0x01: parse_nop;
            8w0x02: parse_ss;
            8w0x03: parse_s;
            8w0x05: parse_sack;
            default: accept;
        }
    }

    state parse_end {
        packet.extract(hdr.options.next.end);
        transition accept;
    }

    state parse_nop {
        packet.extract(hdr.options.next.nop);
        transition start;
    }

    state parse_ss {
        packet.extract(hdr.options.next.ss);
        transition start;
    }

    state parse_s {
        packet.extract(hdr.options.next.s);
        transition start;
    }

    state parse_sack {
        packet.extract(
            hdr.options.next.sack,
            (bit<32>)(8 * packet.lookahead<TcpOptionSackLength>().length - 16));
        transition start;
    }
}

control HeaderUnionIngress(
    inout headers_t hdr,
    inout metadata_t meta,
    in psa_ingress_input_metadata_t istd,
    inout psa_ingress_output_metadata_t ostd) {
    apply {
        hdr.report.setValid();
        hdr.report.parsed_count = meta.parsed_count;
        hdr.report.first_member = meta.first_member;
        send_to_port(ostd, (PortId_t)1);
    }
}

control HeaderUnionEgress(
    inout headers_t hdr,
    inout metadata_t meta,
    in psa_egress_input_metadata_t estd,
    inout psa_egress_output_metadata_t ostd) {
    apply {}
}

control HeaderUnionIngressDeparser(
    packet_out packet,
    out empty_t clone_i2e,
    out empty_t resubmit,
    out empty_t normal,
    inout headers_t hdr,
    in metadata_t meta,
    in psa_ingress_output_metadata_t istd) {
    apply {
        packet.emit(hdr.report);
        packet.emit(hdr.options);
    }
}

control HeaderUnionEgressDeparser(
    packet_out packet,
    out empty_t clone_e2e,
    out empty_t recirculate,
    inout headers_t hdr,
    in metadata_t meta,
    in psa_egress_output_metadata_t ostd,
    in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        packet.emit(hdr.report);
        packet.emit(hdr.options);
    }
}

IngressPipeline(
    HeaderUnionIngressParser(),
    HeaderUnionIngress(),
    HeaderUnionIngressDeparser()) ingress;
EgressPipeline(
    HeaderUnionEgressParser(),
    HeaderUnionEgress(),
    HeaderUnionEgressDeparser()) egress;

PSA_Switch(
    ingress,
    PacketReplicationEngine(),
    egress,
    BufferingQueueingEngine()) main;
