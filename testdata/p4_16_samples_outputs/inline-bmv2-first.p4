#include <core.p4>
#include <v1model.p4>

typedef standard_metadata_t std_meta_t;
header ipv4_t {
}

struct H {
    ipv4_t ipv4;
}

struct M {
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout std_meta_t std_meta) {
    state start {
        transition accept;
    }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control aux(inout M meta, in H hdr) {
    table adjust_lkp_fields {
        key = {
            hdr.ipv4.isValid(): exact @name("hdr.ipv4.$valid$") ;
        }
        actions = {
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        adjust_lkp_fields.apply();
    }
}

control IngressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    aux() do_aux;
    apply {
        do_aux.apply(meta, hdr);
    }
}

control EgressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply {
    }
}

control DeparserI(packet_out b, in H hdr) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;

