#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<32> a;
    bit<8>  b;
    bit<64> c;
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
    @name("ingress.shift") action shift() {
        h.h.c = (bit<64>)(h.h.a >> h.h.b);
        sm.egress_spec = 9w0;
    }
    @name("ingress.t") table t_0 {
        actions = {
            shift();
        }
        const default_action = shift();
    }
    apply {
        t_0.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

