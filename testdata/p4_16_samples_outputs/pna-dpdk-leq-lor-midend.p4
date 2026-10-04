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
        pkt.extract < hdr_t >(h.h);
        transition accept;
    }
}

control PreControlImpl(in Headers hdr, inout Meta meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control ingress(inout Headers h, inout Meta m, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkleqlor48() {
        h.h.a = h.h.a + 8w1;
    }
    @hidden action pnadpdkleqlor51() {
        h.h.b = h.h.b + 8w1;
    }
    @hidden action pnadpdkleqlor54() {
        h.h.c = h.h.c + 8w1;
    }
    @hidden table tbl_pnadpdkleqlor48 {
        actions = {
            pnadpdkleqlor48();
        }
        const default_action = pnadpdkleqlor48();
    }
    @hidden table tbl_pnadpdkleqlor51 {
        actions = {
            pnadpdkleqlor51();
        }
        const default_action = pnadpdkleqlor51();
    }
    @hidden table tbl_pnadpdkleqlor54 {
        actions = {
            pnadpdkleqlor54();
        }
        const default_action = pnadpdkleqlor54();
    }
    apply {
        if (h.h.a <= h.h.b || h.h.c == h.h.d) {
            tbl_pnadpdkleqlor48.apply();
        }
        if (h.h.a >= h.h.b || h.h.c != h.h.d) {
            tbl_pnadpdkleqlor51.apply();
        }
        if (h.h.a <= h.h.b && h.h.c == h.h.d) {
            tbl_pnadpdkleqlor54.apply();
        }
    }
}

control deparser(packet_out pkt, in Headers h, in Meta m, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkleqlor62() {
        pkt.emit < hdr_t >(h.h);
    }
    @hidden table tbl_pnadpdkleqlor62 {
        actions = {
            pnadpdkleqlor62();
        }
        const default_action = pnadpdkleqlor62();
    }
    apply {
        tbl_pnadpdkleqlor62.apply();
    }
}

PNA_NIC < Headers, Meta, Headers, Meta >(p(), PreControlImpl(), ingress(), deparser()) main;
