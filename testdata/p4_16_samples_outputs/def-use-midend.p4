#include <core.p4>
#include <v1model.p4>

typedef standard_metadata_t std_meta_t;
struct H {
}

struct M {
}

parser ParserI(packet_in b, out H h, inout M m, inout std_meta_t s) {
    state start {
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply {
    }
}

control EgressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    @name("EgressI.a") action a_0() {
    }
    @name("EgressI.t") table t {
        key = {
        }
        actions = {
            a_0();
        }
        default_action = a_0();
    }
    apply {
        switch (t.apply().action_run) {
            a_0: {
            }
            default: {
            }
        }

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

control DeparserI(packet_out b, in H hdr) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;

