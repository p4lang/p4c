#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct Headers {
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    @name("p.x") bit<32> x_0;
    state start {
        transition select(x_0) {
            32w0: reject;
            default: noMatch;
        }
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
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

control deparser(packet_out pkt, in Headers h) {
    apply {
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
