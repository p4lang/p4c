#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header hdr {
    bit<16> key;
    bit<32> a;
    bit<32> b;
}

control compute(inout hdr h) {
    action add(bit<32> va, bit<32> vb=0) {
        h.a = h.a + va;
        h.b = h.b + vb;
    }
    table t {
        key = {
            h.key: exact @name("h.key");
        }
        actions = {
            add();
        }
        const default_action = add(32w10, 32w0);
    }
    apply {
        t.apply();
    }
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
    compute() c;
    apply {
        c.apply(h.h);
        sm.egress_spec = 9w0;
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
