#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ipv4_t {
    bit<4>  ihl;
    bit<4>  version;
    bit<16> hdrChecksum;
}

struct H {
    ipv4_t ipv4;
}

struct M {
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        transition accept;
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

struct tuple_0 {
    bit<1> field;
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.ihl == 4w5, { 1w0 }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;

