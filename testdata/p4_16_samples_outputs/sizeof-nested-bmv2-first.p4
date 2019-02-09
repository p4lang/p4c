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

header bitvec_hdr {
    row_t      row;
    varbit<32> v;
}

struct col_t {
    bitvec_hdr bvh;
}

struct local_metadata_t {
    row_t      row0;
    row_t      row1;
    col_t      col;
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;
    varbit<32> v;
}

struct parsed_packet_t {
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;
}

struct tst_t {
    row_t      row0;
    row_t      row1;
    col_t      col;
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;
}

parser parse(packet_in pk, out parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    bit<32> len = 32w0;
    state start {
        pk.extract<bitvec_hdr>(h.bvh0, 32w32);
        pk.extract<bitvec_hdr>(h.bvh1, 32w32);
        len = 32w32;
        len = 32w4;
        pk.extract<bitvec_hdr>(local_metadata.col.bvh, 32w32);
        transition accept;
    }
}

control ingress(inout parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control egress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control deparser(packet_out b, in parsed_packet_t h) {
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

