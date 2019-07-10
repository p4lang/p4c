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
    @name("ingress.t1") table t1_0 {
        actions = {
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action act() {
        sm.egress_spec = 9w1;
    }
    @hidden action act_0() {
        sm.egress_spec = 9w2;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        if (m.b == 1w0) {
            t1_0.apply();
            tbl_act.apply();
        } else {
            t1_0.apply();
            tbl_act_0.apply();
        }
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

