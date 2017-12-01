#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<32> a;
    bit<32> b;
}

control compute(inout hdr h) {
    @name("a") action a_0() {
        h.b = h.a;
    }
    @name("t") table t_0 {
        key = {
            h.a + h.a: exact @name("e") ;
        }
        actions = {
            a_0();
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t_0.apply();
    }
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
    @name("c") compute() c_0;
    apply {
        c_0.apply(h.h);
        sm.egress_spec = 9w0;
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

