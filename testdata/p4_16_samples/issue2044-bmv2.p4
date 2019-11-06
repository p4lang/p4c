#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<32> b;
}

struct Headers {
    hdr h;
}

struct Meta {}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) {
    apply { b.emit(h); }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    table t {
        key = { h.h.b : exact; }
        actions = { NoAction; }
        default_action = NoAction;
    }
    apply {
        if (t.apply().miss) {
            h.h.setInvalid();
        }
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
