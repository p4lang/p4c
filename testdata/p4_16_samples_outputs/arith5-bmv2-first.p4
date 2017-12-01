#include <core.p4>
#include <v1model.p4>

header hdr {
    int<32> a;
    bit<8>  b;
    int<64> c;
}

struct Headers {
    hdr h;
}

struct Meta {
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract<hdr>(h.h);
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
        b.emit<hdr>(h.h);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action shift() {
        h.h.c = (int<64>)(h.h.a >> h.h.b);
        sm.egress_spec = 9w0;
    }
    table t {
        actions = {
            shift();
        }
        const default_action = shift();
    }
    apply {
        t.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

