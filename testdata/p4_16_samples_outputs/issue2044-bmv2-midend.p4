#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header hdr {
    bit<32> b;
}

struct Headers {
    hdr h;
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
        b.emit<hdr>(h.h);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.t") table t_0 {
        key = {
            h.h.b: exact @name("h.h.b");
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action issue2044bmv2l37() {
        h.h.setInvalid();
    }
    @hidden table tbl_issue2044bmv2l37 {
        actions = {
            issue2044bmv2l37();
        }
        const default_action = issue2044bmv2l37();
    }
    apply {
        if (t_0.apply().hit) {
            ;
        } else {
            tbl_issue2044bmv2l37.apply();
        }
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
