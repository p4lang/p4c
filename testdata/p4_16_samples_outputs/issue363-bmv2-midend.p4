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

struct local_metadata_t {
    row_t row;
}

struct mac_t {
    bit<28> lower28Bits;
    bit<20> upper20Bits;
}

struct macDA_t {
    mac_t macDA;
}

header bitvec_hdr {
    bit<5>  g2;
    bit<3>  g3;
    macDA_t mac;
}

struct parsed_packet_t {
    bitvec_hdr bvh;
    bitvec_hdr bvh1;
}

parser parse(packet_in pk, out parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        pk.extract<bitvec_hdr>(h.bvh);
        transition accept;
    }
}

control ingress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @hidden action act() {
        local_metadata.row.alt0.valid = local_metadata.row.alt1.valid;
        local_metadata.row.alt0.port = local_metadata.row.alt1.port;
        local_metadata.row.alt0.valid = 1w1;
        local_metadata.row.alt1.port = local_metadata.row.alt1.port + 7w1;
        clone3<row_t>(CloneType.I2E, 32w0, local_metadata.row);
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

control deparser(packet_out b, in parsed_packet_t h) {
    apply {
        b.emit<bitvec_hdr>(h.bvh);
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

