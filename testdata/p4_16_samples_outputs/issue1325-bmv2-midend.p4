error {
    Unused
}
#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    @hidden action issue1325bmv2l31() {
        mark_to_drop(standard_metadata);
    }
    @hidden table tbl_issue1325bmv2l31 {
        actions = {
            issue1325bmv2l31();
        }
        const default_action = issue1325bmv2l31();
    }
    apply {
        if (local_metadata._test_test_error0 == error.Unused) {
            tbl_issue1325bmv2l31.apply();
        }
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
