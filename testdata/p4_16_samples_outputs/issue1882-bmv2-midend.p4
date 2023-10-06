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

extern ExternCounter {
    ExternCounter();
    void increment();
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.extr") ExternCounter() extr_0;
    @hidden action issue1882bmv2l33() {
        extr_0.increment();
    }
    @hidden table tbl_issue1882bmv2l33 {
        actions = {
            issue1882bmv2l33();
        }
        const default_action = issue1882bmv2l33();
    }
    apply {
        tbl_issue1882bmv2l33.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
