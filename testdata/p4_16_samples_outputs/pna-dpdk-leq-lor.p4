#include <core.p4>

#include <pna.p4>

header hdr_t {
    bit<8> a;
    bit<8> b;
    bit<8> c;
    bit<8> d;
}

struct Headers {
    hdr_t h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers h, inout Meta m, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(h.h);
        transition accept;
    }
}

control PreControlImpl(in Headers hdr, inout Meta meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control ingress(inout Headers h, inout Meta m, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        if (h.h.a <= h.h.b || h.h.c == h.h.d) {
            h.h.a = h.h.a + 1;
        }
        if (h.h.a >= h.h.b || h.h.c != h.h.d) {
            h.h.b = h.h.b + 1;
        }
        if (h.h.a <= h.h.b && h.h.c == h.h.d) {
            h.h.c = h.h.c + 1;
        }
    }
}

control deparser(packet_out pkt, in Headers h, in Meta m, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(h.h);
    }
}

PNA_NIC(p(), PreControlImpl(), ingress(), deparser()) main;
