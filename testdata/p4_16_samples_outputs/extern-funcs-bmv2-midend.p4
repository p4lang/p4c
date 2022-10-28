#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

extern void extern_func(out bit<32> d, bit<32> s);
header hdr {
    bit<32> a;
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
    @hidden action externfuncsbmv2l29() {
        extern_func(h.h.a, 32w0xff);
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_externfuncsbmv2l29 {
        actions = {
            externfuncsbmv2l29();
        }
        const default_action = externfuncsbmv2l29();
    }
    apply {
        tbl_externfuncsbmv2l29.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
