#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    table t1 {
        size = 3;
        actions = {
            NoAction();
        }
        const default_action = NoAction();
    }
    table t2 {
        key = {
            sm.ingress_port: exact @name("sm.ingress_port") ;
        }
        actions = {
            NoAction();
        }
        const entries = {
                        9w0 : NoAction();
        }

        size = 10;
        default_action = NoAction();
    }
    apply {
        t1.apply();
        t2.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

