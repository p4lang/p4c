#include <core.p4>
#include <dpdk/pna.p4>

struct alt_t {
    bit<1> valid;
    bit<7> port;
}

struct row_t {
    alt_t alt0;
    alt_t alt1;
}

header Hdr1 {
    bit<8> _a0;
    bit<1> _row0_alt0_valid1;
    bit<7> _row0_alt0_port2;
    bit<1> _row0_alt1_valid3;
    bit<7> _row0_alt1_port4;
    bit<1> _row1_alt0_valid5;
    bit<7> _row1_alt0_port6;
    bit<1> _row1_alt1_valid7;
    bit<7> _row1_alt1_port8;
}

header Hdr2 {
    bit<16> _b0;
    bit<1>  _row_alt0_valid1;
    bit<7>  _row_alt0_port2;
    bit<1>  _row_alt1_valid3;
    bit<7>  _row_alt1_port4;
}

struct Headers {
    Hdr1 h1;
    Hdr1 u_h1;
    Hdr2 u_h2;
}

struct Meta {
}

parser ParserImpl(packet_in b, out Headers h, inout Meta m, in pna_main_parser_input_metadata_t istd) {
    state start {
        b.extract<Hdr1>(h.h1);
        transition select(h.h1._a0) {
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

control DeparserImpl(packet_out b, in Headers h, in Meta m, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkbvec_union61() {
        b.emit<Hdr1>(h.h1);
        b.emit<Hdr1>(h.u_h1);
        b.emit<Hdr2>(h.u_h2);
    }
    @hidden table tbl_pnadpdkbvec_union61 {
        actions = {
            pnadpdkbvec_union61();
        }
        const default_action = pnadpdkbvec_union61();
    }
    apply {
        tbl_pnadpdkbvec_union61.apply();
    }
}

control ingress(inout Headers h, inout Meta m, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkbvec_union71() {
        h.u_h2.setInvalid();
    }
    @hidden table tbl_pnadpdkbvec_union71 {
        actions = {
            pnadpdkbvec_union71();
        }
        const default_action = pnadpdkbvec_union71();
    }
    apply {
        if (h.u_h2.isValid()) {
            tbl_pnadpdkbvec_union71.apply();
        }
    }
}

control PreControlImpl(in Headers hdr, inout Meta meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<Headers, Meta, Headers, Meta>(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;
