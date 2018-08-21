#include <core.p4>
#include <v1model.p4>

header Hdr {
    bit<32> f1;
}

struct parsed_packet_t {
    Hdr h;
};

struct local_metadata_t {};

error {
    CustomError
}

parser parse(packet_in pk, out parsed_packet_t hdr,
             inout local_metadata_t local_metadata,
             inout standard_metadata_t standard_metadata) {
    state start {
        pk.extract(hdr.h);
        verify(hdr.h.f1 < 10, error.CustomError);
        transition accept;
    }
}

control ingress(inout parsed_packet_t hdr,
                inout local_metadata_t local_metadata,
	        inout standard_metadata_t standard_metadata) {
    apply {
        if (standard_metadata.parser_error == error.NoError) {
            hdr.h.f1 = 0;
        } else if (standard_metadata.parser_error == error.PacketTooShort) {
            hdr.h.setValid();
            hdr.h.f1 = 1;
        } else if (standard_metadata.parser_error == error.CustomError) {
            hdr.h.f1 = 2;
        } else {
            hdr.h.f1 = 3;
        }
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
}

control egress(inout parsed_packet_t hdr,
               inout local_metadata_t local_metadata,
	       inout standard_metadata_t standard_metadata) {
    apply { }
}

control deparser(packet_out b, in parsed_packet_t hdr) {
    apply {
        b.emit(hdr);
    }
}

control verify_checks(inout parsed_packet_t hdr,
                      inout local_metadata_t local_metadata) {
    apply { }
}

control compute_checksum(inout parsed_packet_t hdr,
                         inout local_metadata_t local_metadata) {
    apply { }
}

V1Switch(parse(), verify_checks(), ingress(), egress(),
         compute_checksum(), deparser()) main;
