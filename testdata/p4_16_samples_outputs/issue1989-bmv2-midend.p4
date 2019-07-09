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
    @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.assign_non_const_array_index") action assign_non_const_array_index() {
        hdr.ethernet_stack[1] = hdr.ethernet_stack[meta.color];
    }
    @name("ingress.acl_table") table acl_table_0 {
        actions = {
            assign_non_const_array_index();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.ethernet.etherType: exact @name("hdr.ethernet.etherType") ;
        }
        default_action = NoAction_0();
    }
    apply {
        acl_table_0.apply();
    }
}

V1Switch<headers, test_metadata_t>(parser_stub(), verify_checksum_stub(), ingress(), egress_stub(), compute_checksum_stub(), deparser_stub()) main;

