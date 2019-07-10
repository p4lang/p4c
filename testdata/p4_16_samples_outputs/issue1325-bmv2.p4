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
    test_struct test;
}

parser parse(packet_in pk, out parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control ingress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (local_metadata.test.test_error == error.Unused) {
            mark_to_drop(standard_metadata);
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

V1Switch(parse(), verify_checks(), ingress(), egress(), compute_checksum(), deparser()) main;

