#include <core.p4>
#include <v1model.p4>

typedef standard_metadata_t std_m;
header some_t {
}

struct H {
}

struct M {
    some_t intrinsic_metadata;
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout std_m smeta) {
    state start {
        transition accept;
    }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control IngressI(inout H hdr, inout M meta, inout std_m smeta) {
    apply {
    }
}

control EgressI(inout H hdr, inout M meta, inout std_m smeta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control DeparserI(packet_out b, in H hdr) {
    apply {
    }
}

V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;

