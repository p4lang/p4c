#include <core.p4>
#include <v1model.p4>

header Header {
    bit<32> data;
}

struct M {
}

parser ParserI(packet_in b, out Header p, inout M m, inout standard_metadata_t s) {
    state start {
        b.extract<_>(_);
        transition next;
    }
    state next {
        b.extract<Header>(p);
        transition accept;
    }
}

control IngressI(inout Header p, inout M meta, inout standard_metadata_t s) {
    apply {
    }
}

control EgressI(inout Header hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {
    }
}

control DeparserI(packet_out pk, in Header hdr) {
    apply {
    }
}

control VerifyChecksumI(inout Header hdr, inout M meta) {
    apply {
    }
}

control ComputeChecksumI(inout Header hdr, inout M meta) {
    apply {
    }
}

V1Switch<Header, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;

