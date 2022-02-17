#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct alt_t {
    bit<1> valid;
    bit<7> port;
}

struct row_t {
    alt_t alt0;
    alt_t alt1;
}

struct parsed_packet_t {
}

struct local_metadata_t {
    @field_list(0) 
    bit<1> _row_alt0_valid0;
    @field_list(0) 
    bit<7> _row_alt0_port1;
    @field_list(0) 
    bit<1> _row_alt1_valid2;
    @field_list(0) 
    bit<7> _row_alt1_port3;
}

parser parse(packet_in pk, out parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control ingress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @hidden action issue562bmv2l32() {
        local_metadata._row_alt0_valid0 = local_metadata._row_alt1_valid2;
        local_metadata._row_alt0_port1 = local_metadata._row_alt1_port3;
        local_metadata._row_alt0_valid0 = 1w1;
        local_metadata._row_alt1_port3 = local_metadata._row_alt1_port3 + 7w1;
        clone_preserving_field_list(CloneType.I2E, 32w0, 8w0);
    }
    @hidden table tbl_issue562bmv2l32 {
        actions = {
            issue562bmv2l32();
        }
        const default_action = issue562bmv2l32();
    }
    apply {
        tbl_issue562bmv2l32.apply();
    }
}

control egress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control deparser(packet_out b, in parsed_packet_t hdr) {
    apply {
    }
}

control verifyChecksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control compute_checksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

V1Switch<parsed_packet_t, local_metadata_t>(parse(), verifyChecksum(), ingress(), egress(), compute_checksum(), deparser()) main;

