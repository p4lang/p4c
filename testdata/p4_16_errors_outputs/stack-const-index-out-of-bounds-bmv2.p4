#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header hdr {
    bit<32> f1;
    bit<32> f2;
}

struct Headers {
    hdr[3] hs;
}

struct Meta {
    bit<32> v;
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.hs.next);
        m.v = h.hs.last.f2;
        m.v = m.v + h.hs.last.f2;
        transition select(h.hs.last.f1) {
            0: start;
            default: accept;
        }
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit(h.hs);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        h.hs[-1].setValid();
        h.hs[-1].f1 = 5;
        h.hs[-1].f2 = 8;
        h.hs[0].setValid();
        h.hs[0].f1 = 5;
        h.hs[0].f2 = 8;
        h.hs[2].setValid();
        h.hs[2].f1 = 5;
        h.hs[2].f2 = 8;
        h.hs[3].setValid();
        h.hs[3].f1 = 5;
        h.hs[3].f2 = 8;
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
