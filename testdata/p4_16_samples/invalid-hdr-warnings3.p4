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
        pkt.extract(hdr.h1);
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    Header h1;
    Header h2;

    apply {
        h1.setInvalid();
        h2.setValid();
        h1.data = 0;
        h2.data = 1;

        switch (hdr.h1.data)
        {
            0: { h1.setValid(); h2.setInvalid(); }
            default: { h1.setValid(); h2.setInvalid(); }
        }

        hdr.h1.data = h1.data;
        hdr.h1.data = h2.data;

        switch (hdr.h1.data)
        {
            0: { h1.setValid(); h2.setValid(); }
            default: { h1.setInvalid(); h2.setInvalid(); }
        }

        hdr.h1.data = h1.data;
        hdr.h1.data = h2.data;
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
