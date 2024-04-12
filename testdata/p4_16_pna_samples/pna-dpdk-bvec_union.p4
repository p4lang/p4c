#include <core.p4>
#include <dpdk/pna.p4>

struct alt_t {
  bit<1> valid;
  bit<7> port;
};

struct row_t {
  alt_t alt0;
  alt_t alt1;
};

header Hdr1 {
    bit<8> a;
    row_t row0;
    row_t row1;    
}

header Hdr2 {
    bit<16> b;
    row_t row;
}

header_union U {
    Hdr1 h1;
    Hdr2 h2;
}

struct Headers {
    Hdr1 h1;
    U u;
}

struct Meta {}

parser ParserImpl(packet_in b, out Headers h, inout Meta m,
    in    pna_main_parser_input_metadata_t istd) {
    state start {        
        b.extract(h.h1);
        transition select(h.h1.a) {
            0: getH1;
            default: getH2;
        }
    }

    state getH1 {
        b.extract(h.u.h1);
        transition accept;
    }

    state getH2 {
        b.extract(h.u.h2);
        transition accept;
    }
}

control DeparserImpl(packet_out b, in Headers h, in Meta m,
    in    pna_main_output_metadata_t ostd) {
    apply {
        b.emit(h.h1);
        b.emit(h.u);
    }
}

control ingress(inout Headers h, inout Meta m,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd) {
    apply {
        if (h.u.h2.isValid())
            h.u.h2.setInvalid();
    }
}

control PreControlImpl(
    in    Headers  hdr,
    inout Meta meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {
    }
}

PNA_NIC(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;

