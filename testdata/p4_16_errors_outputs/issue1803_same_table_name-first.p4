#include <core.p4>
#include <v1model.p4>

struct H {
}

struct M {
    bit<32> hash1;
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        transition accept;
    }
}

action empty() {
}
control MyC(inout standard_metadata_t smeta) {
    action drop() {
        mark_to_drop(smeta);
    }
    @name(".t0") table t0 {
        key = {
            smeta.ingress_port: exact @name("smeta.ingress_port") ;
        }
        actions = {
            drop();
            NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        t0.apply();
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    MyC() c1;
    MyC() c2;
    apply {
        c1.apply(smeta);
        c2.apply(smeta);
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {
    }
}

control DeparserI(packet_out pk, in H hdr) {
    apply {
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

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;

