/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <psa.p4>
#include "common_headers.p4"

struct headers {
    ethernet_t eth;
}

parser MyIP(packet_in buffer, out headers hdr, inout empty_t bp,
            in psa_ingress_parser_input_metadata_t c, in empty_t d, in empty_t e) {
    state start {
        buffer.extract(hdr.eth);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out empty_t a, inout empty_t b,
            in psa_egress_parser_input_metadata_t c, in empty_t d, in empty_t e, in empty_t f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers a, inout empty_t bc,
             in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t ostd) {

    ActionSelector(PSA_HashAlgorithm_t.CRC32, 32w1024, 32w16) as;
    
    action fwd(PortId_t port) { send_to_port(ostd, port); }

    table tbl {
        key = {
            a.eth.srcAddr : exact;
            a.eth.dstAddr : selector;
            a.eth.srcAddr : selector;
            a.eth.etherType : selector;
        }
        actions = { NoAction; fwd; }
        psa_implementation = as;
    }

    apply {
        tbl.apply();
    }
}

control MyEC(inout empty_t a, inout empty_t b,
    in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply { }
}

control MyID(packet_out buffer, out empty_t a, out empty_t b, out empty_t c,
    inout headers d, in empty_t e, in psa_ingress_output_metadata_t f) {
    apply {
        buffer.emit(d.eth);
    }
}

control MyED(packet_out buffer, out empty_t a, out empty_t b, inout empty_t c, in empty_t d,
    in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply { }
}

IngressPipeline(MyIP(), MyIC(), MyID()) ip;
EgressPipeline(MyEP(), MyEC(), MyED()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
