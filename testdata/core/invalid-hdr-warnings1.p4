#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header Header {
    bit<32> data;
}

struct H {
    Header h1;
    Header h2;
}

struct M { }

parser ParserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        hdr.h2.data = 1;            // always invalid
        transition next;
    }

    state next {
        if (hdr.h2.data == 1) {     // from start: invalid, from state0: may be invalid  =>  hdr.h2 may be invalid
            hdr.h1.data = 0;        // from start: invalid, from state0: invalid => hdr.h1 is invalid
        }
        pkt.extract(hdr.h1);
        hdr.h2.setInvalid();
        transition select (hdr.h1.data) {   // valid
            0: state0;
            1: state1;
            default: accept;
        }
    }

    state state0 {
        hdr.h1.setInvalid();
        transition select (hdr.h2.data) {  // from next: invalid, from state1: valid => hdr.h2 may be invalid
            2: state1;
            default: next;
        }
    }

    state state1 {
        hdr.h2.data = hdr.h2.data + 1;     // from next: invalid, from state0: may be invalid, from state1: valid
                                           // => hdr.h2 may be invalid
        hdr.h2.setValid();
        transition select (hdr.h2.data) {   // valid
            1: state1;
            2: state0;
            default: accept;
        }
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {
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

V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
