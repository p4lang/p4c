#include <core.p4>
#include <v1model.p4>

struct H { };
struct M {
    bit<32> hash1;
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start { transition accept; }
}

action empty() { }

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    // This is typical of a valid P4_14 program translated into a v1model P4_16
    // program. Having a table and an action with the same name is valid in
    // P4_14. During the translation the compiler ensures that the 2 have
    // different fully-qualified names but preserves the original name for each
    // through a @name annotation.

    @name("do_something")
    action do_something_0() { mark_to_drop(smeta); }

    @name("do_something")
    table do_something {
        key = { smeta.ingress_port : exact; }
        actions = { do_something_0; NoAction; }
        const default_action = NoAction();
    }
    apply {
        do_something.apply();
    }
};

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply { }
};

control DeparserI(packet_out pk, in H hdr) {
    apply { }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply { }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply { }
}

V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(),
         ComputeChecksumI(), DeparserI()) main;
