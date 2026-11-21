#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.eth_hdr);
        transition parse_h;
    }
    state parse_h {
        pkt.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta meta) {
    apply { }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t s) {

    action t0_a0() { /* empty */ }
    action t0_a1() { /* empty */ }
    action t0_a2() { h.h.a = 4; }

    table t0 {
        actions = { t0_a0; t0_a1; t0_a2; }
        size = 8;
        default_action = t0_a0();
    }

    action t1_a0() { /* empty */ }
    action t1_a1() { /* empty */ }
    action t1_a2() { h.h.a = h.h.a + 7; }

    table t1 {
        key = { h.h.a : exact; }
        actions = { t1_a0; t1_a1; t1_a2; }
        size = 8;
        const default_action = t1_a1();
    }

    action t2_a0() { /* empty */ }
    action t2_a1() { /* empty */ }
    action t2_a2() { h.h.a = h.h.a + 13; }

    table t2 {
        key = { h.h.a : exact; }
        actions = { t2_a0; t2_a1; t2_a2; }
        size = 8;
        const entries = {
            0 : t2_a2;
            1 : t2_a2;
            2 : t2_a0;
            3 : t2_a1;
            // implicit default NoAction
        }
    }

    action free_a0() { /* empty */ }
    action free_a1() { /* empty */ }
    action free_nested() { /* empty */ }
    action free_a2() { h.h.a = h.h.a + 11; free_nested(); }

    apply {
        if (h.h.isValid()) {
            // check action coverage tracking on default actions
            t0.apply();
            // check action coverage tracking on non-default actions
            t1.apply();
            // check action coverage tracking on const entries
            t2.apply();
            // check action coverage on freestanding actions
            if (h.h.a > 8) {
                free_a0();
            } else if (h.h.b > 8) {
                free_a1();
            } else {
                free_a2();
            }
        }
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply { }
}

control update(inout Headers h, inout Meta m) {
    apply { }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
