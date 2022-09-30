#include <core.p4>
#include <v1model.p4>

struct H { }

struct M { }

parser ParserI(packet_in packet, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        transition s0;
    }

    state s0 {
        transition s1;
    }

    state s1 {
        packet.advance(16);
        transition select(packet.lookahead<bit<16>>()) {
            0 : s1;
            default : s2;
        }
    }

    state s2 {
        transition s3;
    }

    state s3 {
        packet.advance(16);
        transition select(packet.lookahead<bit<16>>()) {
            0 : s1;
            1 : s2;
            default : s4;
        }
    }

    state s4 {
        transition accept;
    }
}

control Aux(inout M meta) {
    apply {
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {
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