#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct Headers {
}

struct Meta {
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

register<bit<8>>(32w2) r;

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.x") bit<8> x_0;
    @hidden action issue1097bmv2l19() {
        r.read(x_0, 32w0);
    }
    @hidden table tbl_issue1097bmv2l19 {
        actions = {
            issue1097bmv2l19();
        }
        const default_action = issue1097bmv2l19();
    }
    apply {
        tbl_issue1097bmv2l19.apply();
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

