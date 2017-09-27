#include <core.p4>
#include <v1model.p4>

struct H { };
struct M {
    bit<32> hash1;
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t
    smeta) {
    state start { transition accept; }
}

action empty() { }

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {

    action drop() { smeta.drop = 1; }

    action_selector (HashAlgorithm.identity, 32w1024, 32w10) as;
    apply { }
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
