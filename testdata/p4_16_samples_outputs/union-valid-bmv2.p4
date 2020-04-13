#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header Hdr1 {
    bit<32> a;
}

header Hdr2 {
    bit<64> b;
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

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.h1);
        transition select(h.h1.a) {
            0: getH1;
            default: getH1;
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
        b.emit(h.h1);
        b.emit(h.u);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action a() {
    }
    table t {
        key = {
            h.u.isValid(): exact;
        }
        actions = {
            a;
        }
        default_action = a;
    }
    apply {
        t.apply();
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

