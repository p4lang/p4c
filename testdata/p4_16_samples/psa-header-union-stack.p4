/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <bmv2/psa.p4>

#include "spec-ex09.p4"

header empty_t {}
struct metadata_t {}
struct headers_t {
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

parser HeaderUnionEgressParser(
    packet_in packet,
    out headers_t hdr,
    inout metadata_t meta,
    in psa_egress_parser_input_metadata_t estd,
    in empty_t normal,
    in empty_t clone_i2e,
    in empty_t clone_e2e) {
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
        packet.emit(hdr.options[0]);
        packet.emit(hdr.options[1]);
        packet.emit(hdr.options[2]);
        packet.emit(hdr.options[3]);
        packet.emit(hdr.options[4]);
        packet.emit(hdr.options[5]);
        packet.emit(hdr.options[6]);
        packet.emit(hdr.options[7]);
        packet.emit(hdr.options[8]);
        packet.emit(hdr.options[9]);
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
        packet.emit(hdr.options[0]);
        packet.emit(hdr.options[1]);
        packet.emit(hdr.options[2]);
        packet.emit(hdr.options[3]);
        packet.emit(hdr.options[4]);
        packet.emit(hdr.options[5]);
        packet.emit(hdr.options[6]);
        packet.emit(hdr.options[7]);
        packet.emit(hdr.options[8]);
        packet.emit(hdr.options[9]);
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
