#include <core.p4>
#include <v1model.p4>

header Hdr {
    bit<8> x;
}

struct Headers {
    Hdr[2] h1;
    Hdr[2] h2;
}

struct Meta {
}

parser P(packet_in p, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        p.extract<Hdr>(h.h1.next);
        p.extract<Hdr>(h.h1.next);
        h.h2 = h.h1;
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
        b.emit<Hdr>(h.h1[0]);
        b.emit<Hdr>(h.h1[1]);
        b.emit<Hdr>(h.h2[0]);
        b.emit<Hdr>(h.h2[1]);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action act() {
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

V1Switch<Headers, Meta>(P(), vrfy(), ingress(), egress(), update(), deparser()) main;

