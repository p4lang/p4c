#include <core.p4>
#include <v1model.p4>

header hdr {
}

struct Headers {
}

struct Meta {
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
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
        b.emit(h);
    }
}

extern ExternCounter {
    ExternCounter(int size);
    void increment();
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    ExternCounter(5) extr;
    apply {
        extr.increment();
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

