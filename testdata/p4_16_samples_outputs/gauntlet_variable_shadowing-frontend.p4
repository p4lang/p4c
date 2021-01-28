#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header hdr {
    bit<32> a;
    bit<32> b;
}

struct Headers {
    hdr h;
}

struct Meta {
    bit<8> test;
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
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.tmp") bit<9> tmp_0;
    @name("ingress.c.a") action c_a_0() {
        h.h.b = h.h.a;
    }
    @name("ingress.c.t") table c_t {
        key = {
            h.h.a + h.h.a: exact @name("e") ;
        }
        actions = {
            c_a_0();
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        m.test = 8w1;
        tmp_0 = 9w1;
        c_t.apply();
        sm.egress_spec = tmp_0;
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

