#include <core.p4>

#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header Hdr {
    bit<8> a;
}

struct Headers {
    Hdr op;
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

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action issue5558bmv2l32() {
        h.op.a[3:0] = 8w0x0[h.op.a+:4];
    }
    @hidden table tbl_issue5558bmv2l32 {
        actions = {
            issue5558bmv2l32();
        }
        const default_action = issue5558bmv2l32();
    }
    apply {
        tbl_issue5558bmv2l32.apply();
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<Hdr>(h.op);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
