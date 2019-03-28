error {
    Unused
}
#include <core.p4>
#include <v1model.p4>

struct parsed_packet_t {
}

struct test_struct {
    error test_error;
}

struct local_metadata_t {
    error _test_test_error0;
}

parser parse(packet_in pk, out parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control ingress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name(".markToDrop") action markToDrop() {
        standard_metadata.egress_spec = 9w511;
        standard_metadata.mcast_grp = 16w0;
    }
    @hidden table tbl_markToDrop {
        actions = {
            markToDrop();
        }
        const default_action = markToDrop();
    }
    apply {
        if (local_metadata._test_test_error0 == error.Unused) 
            tbl_markToDrop.apply();
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

control verify_checks(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control compute_checksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

V1Switch<parsed_packet_t, local_metadata_t>(parse(), verify_checks(), ingress(), egress(), compute_checksum(), deparser()) main;

