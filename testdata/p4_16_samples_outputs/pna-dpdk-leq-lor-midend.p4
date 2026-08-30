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
        pkt.extract<hdr_t>(h.h);
        transition accept;
    }
}

control PreControlImpl(in Headers hdr, inout Meta meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control ingress(inout Headers h, inout Meta m, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkleqlor46() {
        h.h.a = h.h.a + 8w1;
    }
    @hidden action pnadpdkleqlor49() {
        h.h.b = h.h.b + 8w1;
    }
    @hidden action pnadpdkleqlor52() {
        h.h.c = h.h.c + 8w1;
    }
    @hidden table tbl_pnadpdkleqlor46 {
        actions = {
            pnadpdkleqlor46();
        }
        const default_action = pnadpdkleqlor46();
    }
    @hidden table tbl_pnadpdkleqlor49 {
        actions = {
            pnadpdkleqlor49();
        }
        const default_action = pnadpdkleqlor49();
    }
    @hidden table tbl_pnadpdkleqlor52 {
        actions = {
            pnadpdkleqlor52();
        }
        const default_action = pnadpdkleqlor52();
    }
    apply {
        if (h.h.a <= h.h.b || h.h.c == h.h.d) {
            tbl_pnadpdkleqlor46.apply();
        }
        if (h.h.a >= h.h.b || h.h.c != h.h.d) {
            tbl_pnadpdkleqlor49.apply();
        }
        if (h.h.a <= h.h.b && h.h.c == h.h.d) {
            tbl_pnadpdkleqlor52.apply();
        }
    }
}

control deparser(packet_out pkt, in Headers h, in Meta m, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkleqlor60() {
        pkt.emit<hdr_t>(h.h);
    }
    @hidden table tbl_pnadpdkleqlor60 {
        actions = {
            pnadpdkleqlor60();
        }
        const default_action = pnadpdkleqlor60();
    }
    apply {
        tbl_pnadpdkleqlor60.apply();
    }
}

PNA_NIC<Headers, Meta, Headers, Meta>(p(), PreControlImpl(), ingress(), deparser()) main;
