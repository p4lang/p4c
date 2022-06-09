#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header hdr {
    bit<32> a;
    bit<32> b;
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
    @name("ingress.add") action add() {
        h.h.c = (bit<64>)(h.h.a + h.h.b);
        sm.egress_spec = 9w0;
    }
    @name("ingress.add") action add_1() {
        h.h.c = (bit<64>)(h.h.a + h.h.b);
        sm.egress_spec = 9w0;
    }
    @name("ingress.sub") action sub() {
    }
    @name("ingress.sub") action sub_1() {
    }
    @name("ingress.t") table t_0 {
        actions = {
            add();
            sub();
        }
        const default_action = add();
    }
    @name("ingress.t1") table t1_0 {
        actions = {
            sub_1();
        }
        const default_action = sub_1();
    }
    @name("ingress.t2") table t2_0 {
        actions = {
            add_1();
        }
        const default_action = add_1();
    }
    apply {
        switch (t_0.apply().action_run) {
            add: {
            }
            default: {
                t1_0.apply();
            }
        }
        t2_0.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

