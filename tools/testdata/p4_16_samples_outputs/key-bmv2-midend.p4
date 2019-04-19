#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<32> a;
    bit<32> b;
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
    bit<32> key_0;
    @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.c.a") action c_a_0() {
        h.h.b = h.h.a;
    }
    @name("ingress.c.t") table c_t {
        key = {
            key_0: exact @name("e") ;
        }
        actions = {
            c_a_0();
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action act() {
        key_0 = h.h.a + h.h.a;
    }
    @hidden action act_0() {
        sm.egress_spec = 9w0;
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
        tbl_act.apply();
        c_t.apply();
        tbl_act_0.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

