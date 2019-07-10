#include <core.p4>
#include <v1model.p4>

enum meter_color_t {
    COLOR_GREEN,
    COLOR_RED,
    COLOR_YELLOW
}

struct test_metadata_t {
    bit<32>       color;
    meter_color_t enum_color;
    bit<32>       other_metadata;
    bit<16>       smaller_metadata;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers {
    ethernet_t    ethernet;
    ethernet_t    ethernet2;
    ethernet_t[2] ethernet_stack;
}

parser parser_stub(packet_in packet, out headers hdr, inout test_metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control egress_stub(inout headers hdr, inout test_metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress_stub(inout headers hdr, inout test_metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control deparser_stub(packet_out packet, in headers hdr) {
    apply {
    }
}

control verify_checksum_stub(inout headers hdr, inout test_metadata_t meta) {
    apply {
    }
}

control compute_checksum_stub(inout headers hdr, inout test_metadata_t meta) {
    apply {
    }
}

control ingress(inout headers hdr, inout test_metadata_t meta, inout standard_metadata_t standard_metadata) {
    action assign_non_const_array_index() {
        hdr.ethernet_stack[1] = hdr.ethernet_stack[meta.color];
    }
    table acl_table {
        actions = {
            assign_non_const_array_index;
        }
        key = {
            hdr.ethernet.etherType: exact;
        }
    }
    apply {
        acl_table.apply();
    }
}

V1Switch(parser_stub(), verify_checksum_stub(), ingress(), egress_stub(), compute_checksum_stub(), deparser_stub()) main;

