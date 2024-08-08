#include <v1model.p4>

header opt_t {
    bit<8> opt;
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> a1;
    bit<8> b;
}

header_union U {
    ethernet_t e;
    H h;
}

struct Headers {
    opt_t opt;
    U u;
}

struct Meta {}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.opt);
        transition select(h.opt.opt) {
            0x0: parse_e;
            0x1: parse_h;
            default: accept;
        }
    }

    state parse_e {
        pkt.extract(h.u.e);
        transition invalidate;
    }

    state parse_h {
        pkt.extract(h.u.h);
        transition invalidate;
    }

    state invalidate {
        h.u.e.setInvalid();
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta meta) {
    apply { }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply {
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
