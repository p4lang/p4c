#include <v1model.p4>

struct HasBool {
    bool x;
}

struct parsed_packet_t {}
struct local_metadata_t {}

parser parse(packet_in pk, out parsed_packet_t h, inout local_metadata_t local_metadata,
             inout standard_metadata_t standard_metadata) {
    state start {
	transition accept;
    }
}

control ingress(inout parsed_packet_t h, inout local_metadata_t local_metadata,
                inout standard_metadata_t standard_metadata) {
    apply {
        HasBool b;
        b.x = true;
        clone3(CloneType.I2E, 0, b);
    }
}

control egress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata,
               inout standard_metadata_t standard_metadata) {
    apply { }
}

control deparser(packet_out b, in parsed_packet_t h) {
    apply { }
}

control verifyChecksum(inout parsed_packet_t hdr,
inout local_metadata_t local_metadata) {
    apply { }
}

control compute_checksum(inout parsed_packet_t hdr,
                         inout local_metadata_t local_metadata) {
    apply { }
}

V1Switch(parse(), verifyChecksum(), ingress(), egress(),
compute_checksum(), deparser()) main;
