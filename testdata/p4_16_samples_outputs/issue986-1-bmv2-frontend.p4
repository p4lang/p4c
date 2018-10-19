#include <core.p4>
#include <v1model.p4>

struct Meta {
    bit<1> b;
}

struct Headers {
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
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.t1") table t1_0 {
        actions = {
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name("ingress.t2") table t2_0 {
        actions = {
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    apply {
        if (m.b == 1w0) 
            t1_0.apply();
        else 
            t1_0.apply();
        t2_0.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

