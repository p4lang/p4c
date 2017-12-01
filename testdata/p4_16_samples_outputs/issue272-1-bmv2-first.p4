#include <core.p4>
#include <v1model.p4>

typedef standard_metadata_t std_meta_t;
struct some_meta_t {
    bool flag;
}

struct H {
}

struct M {
    some_meta_t some_meta;
}

control DeparserI(packet_out packet, in H hdr) {
    apply {
    }
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

control IngressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply {
        meta.some_meta.flag = true;
    }
}

control EgressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;

