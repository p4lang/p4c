#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

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

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
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
        b.emit<Hdr1>(h.h1);
        b.emit<Hdr1>(h.u_h1);
        b.emit<Hdr2>(h.u_h2);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action union2bmv2l75() {
        h.u_h2.setInvalid();
        h.u_h1.setValid();
        h.u_h2.setInvalid();
        h.u_h1.setValid();
        h.u_h1.a = 8w0xff;
        h.u_h2.setInvalid();
    }
    @hidden table tbl_union2bmv2l75 {
        actions = {
            union2bmv2l75();
        }
        const default_action = union2bmv2l75();
    }
    apply {
        if (h.u_h2.isValid()) {
            tbl_union2bmv2l75.apply();
        }
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
