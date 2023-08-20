#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<32> a;
    bit<32> b;
}

struct Headers {
    H h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp1") H tmp1_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_action") action simple_action() {
        tmp1_0.setInvalid();
        if (tmp1_0.a != 32w10) {
            tmp1_0.a = tmp1_0.a + 32w10;
        }
        h.h.a = tmp1_0.a;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.h.b: exact @name("key");
        }
        actions = {
            simple_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        simple_table_0.apply();
        simple_table_0.apply();
    }
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

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
