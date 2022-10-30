#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct H {
}

struct M {
}

parser ParserI(packet_in b, out H h, inout M m, inout standard_metadata_t s) {
    state start {
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t std_meta) {
    @name("EgressI.a") action a() {
    }
    @name("EgressI.t") table t_0 {
        actions = {
            a();
        }
        default_action = a();
    }
    apply {
        switch (t_0.apply().action_run) {
            a: {
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
