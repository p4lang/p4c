#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header Header {
    bit<32> data;
}

struct H {
    Header[2] h1;
    Header[2] h2;
}

struct M { }

parser ParserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        pkt.extract(hdr.h1[0]);
        hdr.h1[0].data = 1;
        hdr.h1[1].data = 1;
        transition init;
    }

    state init {
        pkt.extract(hdr.h1[1]);
        hdr.h1[0].data = 1;
        hdr.h1[1].data = 1;
        transition next;
    }

    state next {
        hdr.h1[0].setInvalid();
        pkt.extract(hdr.h1.next);
        hdr.h1[0].data = 1;
        hdr.h1[1].data = 1;
        hdr.h1[0].setInvalid();
        transition select (hdr.h1[1].data) {
            0: init;
            default: accept;
        }
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    H h;
    H h_copy;
    H h_copy2;

    action validateHeader(inout Header hd) {
        hd.setValid();
    }

    action invalidateHeader(inout Header hd) {
        hd.setInvalid();
    }

    action invalidateStack(inout Header[2] stack) {
        invalidateHeader(stack[0]);
        invalidateHeader(stack[1]);
    } 

    apply {
        validateHeader(h.h1[0]);
        h.h1[0].data = 1;
        h.h1[1] = h.h1[0];
        h.h1[1].data = 1;

        h.h2 = h.h1;
        h.h2[0].data = 1;
        h.h2[1].data = 1;

        h_copy = h;
        h_copy.h1[0].data = 1;
        h_copy.h1[1].data = 1;
        h_copy.h2[0].data = 1;
        h_copy.h2[1].data = 1;

        invalidateHeader(h.h2[0]);
        h_copy2 = { h.h1, h.h2 };
        h_copy2.h1[0].data = 1;
        h_copy2.h1[1].data = 1;
        h_copy2.h2[0].data = 1;
        h_copy2.h2[1].data = 1;

        bit i = 1;
        h_copy2.h2[i] = h.h2[0];    // no effect in analysis because h.h2[0] is invalid

        h_copy2.h1[0].data = 1;
        h_copy2.h1[1].data = 1;
        h_copy2.h2[0].data = 1;
        h_copy2.h2[1].data = 1;

        validateHeader(h.h2[i]);    // h.h2[0] and h.h2[1] considered valid from this point
        h_copy2.h2[i] = h.h2[0];

        h_copy2.h1[0].data = 1;
        h_copy2.h1[1].data = 1;
        h_copy2.h2[0].data = 1;
        h_copy2.h2[1].data = 1;

        invalidateStack(h.h1);
        invalidateStack(h_copy.h1);
        h_copy.h1[0] = h.h1[i];     // h_copy.h1[0] is invalid from this point because all fields
                                    // of h.h1 are invalid, so h.h1[i] must be invalid too
        h_copy.h1[0].data = 1;
        h_copy.h1[1].data = 1;

        h.h1[1].setValid();
        h_copy.h1[0] = h.h1[i];     // h.h1[i] could be valid or invalid here depending on i,
                                    // so h_copy.h1[0] is considered valid from this point in
                                    // order to avoid false positives
        h_copy.h1[0].data = 1;
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

