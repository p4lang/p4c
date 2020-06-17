#include <core.p4>
#include <v1model.p4>

header foo_t {
    bit<8> f;
}

struct H {
    foo_t foo;
};
struct M { };

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start { transition accept; }
}

control Aux(inout H hdr, inout M meta) {
    action x(bit<8> a, bit<8> b) {
        hdr.foo.f = a + b;
    }

    action y(bit<8> a, bit<8> b) {
        x(a);
        //x(a, 0);
    }

    table z {
        actions = { x; y; }
    }

    apply {
        z.apply();
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    Aux() aux;
    apply {
        aux.apply(hdr, meta);
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply { }
}

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