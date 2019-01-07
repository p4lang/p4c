#include <core.p4>
#include <v1model.p4>

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
        b.extract<Hdr1>(h.h1);
        transition select(h.h1._a0) {
            8w0: getH1;
            default: getH2;
        }
    }
    state getH1 {
        b.extract<Hdr1>(h.u.h1);
        transition accept;
    }
    state getH2 {
        b.extract<Hdr2>(h.u.h2);
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
        b.emit<Hdr1>(h.u.h1);
        b.emit<Hdr2>(h.u.h2);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action act() {
        h.u.h2.setInvalid();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        if (h.u.h2.isValid()) 
            tbl_act.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

