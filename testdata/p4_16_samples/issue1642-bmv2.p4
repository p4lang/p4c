#include <v1model.p4>

header short {
    bit<32> f;
}

struct alt_t {
  bit<1> valid;
  bit<7> port;
};

struct row_t {
  alt_t alt0;
  alt_t alt1;
};

struct parsed_packet_t {};

struct local_metadata_t {
    short s;
    row_t row;
};

parser parse(packet_in pk, out parsed_packet_t hdr,
             inout local_metadata_t local_metadata,
             inout standard_metadata_t standard_metadata) {
  state start {
    transition accept;
  }
}

control ingress(inout parsed_packet_t hdr,
                inout local_metadata_t local_metadata,
	        inout standard_metadata_t standard_metadata) {
    apply {
        local_metadata.s.setValid();
        local_metadata.s.f = 0;
        local_metadata.row.alt0 = local_metadata.row.alt1;
        local_metadata.row.alt0.valid = 1;
        local_metadata.row.alt1.port = local_metadata.row.alt1.port + 1;
        clone3(CloneType.I2E, 0, local_metadata.row);
    }
}

control egress(inout parsed_packet_t hdr,
               inout local_metadata_t local_metadata,
	       inout standard_metadata_t standard_metadata) {
  apply { }
}

control deparser(packet_out b, in parsed_packet_t hdr) {
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
