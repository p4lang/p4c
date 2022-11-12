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
    @hidden action issue1043bmv2l33() {
        resubmit_preserving_field_list(8w0);
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_issue1043bmv2l33 {
        actions = {
            issue1043bmv2l33();
        }
        const default_action = issue1043bmv2l33();
    }
    apply {
        tbl_issue1043bmv2l33.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
