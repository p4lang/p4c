#include <v1model.p4>
#include "../vendor_hu.p4"

header Hdr3 {
    bit<16> c;
}

header_union U override {
    Hdr3 h3;
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.u.h3);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        h.u.h1.x = true;
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit(h.h1);
        b.emit(h.u);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        if (h.u.h2.isValid())
            h.u.h2.setInvalid();
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
