#include <core.p4>
#include <dpdk/pna.p4>

header Hdr1 {
    bit<8> a;
}

header Hdr2 {
    bit<16> b;
}

struct Headers {
    Hdr1 h1;
    Hdr1 u_h1;
    Hdr2 u_h2;
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
        b.extract<Hdr1>(h.u_h1);
        transition accept;
    }
    state getH2 {
        b.extract<Hdr2>(h.u_h2);
        transition accept;
    }
}

control deparser(packet_out b, in Headers h, in Meta m, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkunionbmv2l64() {
        b.emit<Hdr1>(h.h1);
        b.emit<Hdr1>(h.u_h1);
        b.emit<Hdr2>(h.u_h2);
    }
    @hidden table tbl_pnadpdkunionbmv2l64 {
        actions = {
            pnadpdkunionbmv2l64();
        }
        const default_action = pnadpdkunionbmv2l64();
    }
    apply {
        tbl_pnadpdkunionbmv2l64.apply();
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
