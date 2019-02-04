#include <core.p4>
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
    bit<1> _row_alt0_valid0;
    bit<7> _row_alt0_port1;
    bit<1> _row_alt1_valid2;
    bit<7> _row_alt1_port3;
}

parser parse(packet_in pk, out parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control ingress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @hidden action act() {
        local_metadata._row_alt0_valid0 = local_metadata._row_alt1_valid2;
        local_metadata._row_alt0_port1 = local_metadata._row_alt1_port3;
        local_metadata._row_alt0_valid0 = 1w1;
        local_metadata._row_alt1_port3 = local_metadata._row_alt1_port3 + 7w1;
        clone3<row_t>(CloneType.I2E, 32w0, {{1w1,local_metadata._row_alt0_port1},{local_metadata._row_alt1_valid2,local_metadata._row_alt1_port3}});
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
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

control verify_checksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control compute_checksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

V1Switch<parsed_packet_t, local_metadata_t>(parse(), verify_checksum(), ingress(), egress(), compute_checksum(), deparser()) main;

