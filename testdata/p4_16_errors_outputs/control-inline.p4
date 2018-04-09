#include <core.p4>
#include <v1model.p4>

typedef standard_metadata_t std_m;
struct H {
}

struct M {
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout std_m smeta) {
    state start {
        transition accept;
    }
}

control VC(in H hdr, inout M meta) {
    apply {
    }
}

control Main(inout H hdr, inout M meta, inout std_m smeta) {
    apply {
        VC() vc;
        vc.apply(hdr, meta);
    }
}

control CC(inout H hdr, inout M meta) {
    apply {
    }
}

control DeparserI(packet_out b, in H hdr) {
    apply {
    }
}

V1Switch(ParserI(), VC(), Main(), Main(), CC(), DeparserI()) main;

