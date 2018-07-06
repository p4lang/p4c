#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<32> a;
}

control compute(inout hdr h) {
    bit<8> n = 0;
    apply {
        if (!h.isValid()) {
            return;
        }
        if (n > 0) {
            h.setValid();
        }
    }
}

struct Headers {
    hdr h;
}

struct Meta {
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.h);
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
        b.emit(h.h);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    compute() c;
    apply {
        c.apply(h.h);
        sm.egress_spec = 0;
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

