/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> ether_type;
}
struct headers_t { ethernet_t eth; }
struct metadata_t { bit<8> value; }
parser RtSmithParser(packet_in packet, out headers_t hdr, inout metadata_t meta,
              inout standard_metadata_t sm) {
    state start { packet.extract(hdr.eth); transition accept; }
}
control RtSmithVerifyChecksum(inout headers_t hdr, inout metadata_t meta) { apply {} }
control RtSmithIngress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    action set_value(bit<8> value) { meta.value = value; }
    table exact_table {
        key = { hdr.eth.ether_type : exact; }
        actions = { set_value; @defaultonly NoAction; }
        default_action = NoAction();
        size = 8;
    }
    table lpm_table {
        key = { hdr.eth.ether_type : lpm; }
        actions = { set_value; @defaultonly NoAction; }
        default_action = NoAction();
        size = 8;
    }
    table ternary_table {
        key = { hdr.eth.ether_type : ternary; }
        actions = { set_value; @defaultonly NoAction; }
        default_action = NoAction();
        size = 8;
    }
    table range_table {
        key = { hdr.eth.ether_type : range; }
        actions = { set_value; @defaultonly NoAction; }
        default_action = NoAction();
        size = 8;
    }
    table optional_table {
        key = { hdr.eth.ether_type : optional; }
        actions = { set_value; @defaultonly NoAction; }
        default_action = NoAction();
        size = 8;
    }
    apply {
        if (!hdr.eth.isValid()) {
            mark_to_drop(sm);
            exit;
        }
        meta.value = 0;
        exact_table.apply();
        lpm_table.apply();
        ternary_table.apply();
        range_table.apply();
        optional_table.apply();
        sm.egress_spec = 1;
    }
}
control RtSmithEgress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    apply {}
}
control RtSmithComputeChecksum(inout headers_t hdr, inout metadata_t meta) { apply {} }
control RtSmithDeparser(packet_out packet, in headers_t hdr) { apply { packet.emit(hdr.eth); } }
V1Switch(RtSmithParser(), RtSmithVerifyChecksum(), RtSmithIngress(), RtSmithEgress(), RtSmithComputeChecksum(), RtSmithDeparser()) main;
