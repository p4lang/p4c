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
    H tmp;
    M tmp_0;
    standard_metadata_t tmp_1;
    @name("a") action a_0() {
    }
    @name("t") table t_0() {
        key = {
        }
        actions = {
            a_0();
        }
        default_action = a_0();
    }
    @name("do") IngressI() do_0;
    apply {
        switch (t_0.apply().action_run) {
            a_0: {
                return;
            }
            default: {
                tmp_1 = std_meta;
                do_0.apply(tmp, tmp_0, tmp_1);
                std_meta = tmp_1;
            }
        }

    }
}

control VerifyChecksumI(in H hdr, inout M meta) {
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
