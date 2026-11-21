/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test case for Predication::EmptyStatementRemover bug fix.
 *
 * This program triggers the bug where EmptyStatementRemover
 * eliminates the body of a for-loop that is inside an if-statement
 * within an action. The Predication pass transforms the assignment
 * inside the loop into a Mux expression and replaces it with
 * EmptyStatement. The EmptyStatementRemover then tries to remove
 * it, which would leave the loop body as nullptr.
 *
 * The fix ensures that ForStatement and ForInStatement bodies
 * are replaced with EmptyStatement instead of being left as nullptr.
 */

#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY { }

typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
    bit<32> counter;
}

parser MyIP(
    packet_in buffer,
    out headers_t hdr,
    inout metadata_t meta,
    in psa_ingress_parser_input_metadata_t c,
    in EMPTY d,
    in EMPTY e) {

    state start {
        buffer.extract(hdr.ethernet);
        meta.counter = 0;
        transition accept;
    }
}

parser MyEP(
    packet_in buffer,
    out EMPTY a,
    inout EMPTY b,
    in psa_egress_parser_input_metadata_t c,
    in EMPTY d,
    in EMPTY e,
    in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(
    inout headers_t hdr,
    inout metadata_t meta,
    in psa_ingress_input_metadata_t c,
    inout psa_ingress_output_metadata_t d) {

    action increment_in_loop() {
        // This if-statement triggers the Predication pass
        if (meta.counter == 0) {
            // This for-loop body contains only an assignment that gets
            // transformed by Predication into a Mux expression and
            // replaced with EmptyStatement. Without the fix,
            // EmptyStatementRemover would set loop body to nullptr.
            for (bit<32> i = 0; i < 10; i = i + 1) {
                meta.counter = meta.counter + 1;
            }
        }
    }

    table tbl {
        key = {
            hdr.ethernet.srcAddr : exact;
        }
        actions = { NoAction; increment_in_loop; }
    }

    apply {
        tbl.apply();
    }
}

control MyEC(
    inout EMPTY a,
    inout EMPTY b,
    in psa_egress_input_metadata_t c,
    inout psa_egress_output_metadata_t d) {
    apply { }
}

control MyID(
    packet_out buffer,
    out EMPTY a,
    out EMPTY b,
    out EMPTY c,
    inout headers_t hdr,
    in metadata_t meta,
    in psa_ingress_output_metadata_t f) {
    apply {
        buffer.emit(hdr.ethernet);
    }
}

control MyED(
    packet_out buffer,
    out EMPTY a,
    out EMPTY b,
    inout EMPTY c,
    in EMPTY d,
    in psa_egress_output_metadata_t e,
    in psa_egress_deparser_input_metadata_t f) {
    apply { }
}

IngressPipeline(MyIP(), MyIC(), MyID()) ip;
EgressPipeline(MyEP(), MyEC(), MyED()) ep;

PSA_Switch(
    ip,
    PacketReplicationEngine(),
    ep,
    BufferingQueueingEngine()) main;
