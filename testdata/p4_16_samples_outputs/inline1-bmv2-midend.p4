#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ipv4_t {
}

struct H {
    ipv4_t ipv4;
}

struct M {
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t std_meta) {
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

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t std_meta) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("IngressI.do_aux.adjust_lkp_fields") table do_aux_adjust_lkp_fields {
        key = {
            hdr.ipv4.isValid(): exact @name("hdr.ipv4.$valid$");
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        do_aux_adjust_lkp_fields.apply();
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control DeparserI(packet_out b, in H hdr) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
