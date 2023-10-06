#include <core.p4>
#include <dpdk/pna.p4>

header Hdr1 {
    bit<8> a;
}

header Hdr2 {
    bit<16> b;
}

header_union U {
    Hdr1 h1;
    Hdr2 h2;
}

struct Headers {
    Hdr1 h1;
    U    u;
}

struct Meta {
}

parser p(packet_in b, out Headers h, inout Meta m, in pna_main_parser_input_metadata_t istd) {
    state start {
        b.extract<Hdr1>(h.h1);
        transition select(h.h1.a) {
            8w0: getH1;
            default: getH2;
        }
    }
    state getH1 {
        b.extract<Hdr1>(h.u.h1);
        transition accept;
    }
    state getH2 {
        b.extract<Hdr2>(h.u.h2);
        transition accept;
    }
}

control deparser(packet_out b, in Headers h, in Meta m, in pna_main_output_metadata_t ostd) {
    apply {
        b.emit<Hdr1>(h.h1);
        b.emit<U>(h.u);
    }
}

control ingress(inout Headers h, inout Meta m, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
    }
}

control PreControlImpl(in Headers hdr, inout Meta meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<Headers, Meta, Headers, Meta>(p(), PreControlImpl(), ingress(), deparser()) main;
