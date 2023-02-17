#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header hdr {
    bit<32> f1;
    bit<32> f2;
}

struct Headers {
    hdr[3] hs;
}

struct Meta {
    bit<32> v;
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state stateOutOfBound {
        verify(false, error.StackOutOfBounds);
        transition reject;
    }
    state start {
        b.extract<hdr>(h.hs[32w0]);
        m.v = h.hs[32w0].f2;
        m.v = h.hs[32w0].f2 + h.hs[32w0].f2;
        transition select(h.hs[32w0].f1) {
            32w0: start1;
            default: accept;
        }
    }
    state start1 {
        b.extract<hdr>(h.hs[32w1]);
        m.v = h.hs[32w1].f2;
        m.v = h.hs[32w1].f2 + h.hs[32w1].f2;
        transition select(h.hs[32w1].f1) {
            32w0: start2;
            default: accept;
        }
    }
    state start2 {
        b.extract<hdr>(h.hs[32w2]);
        m.v = h.hs[32w2].f2;
        m.v = h.hs[32w2].f2 + h.hs[32w2].f2;
        transition select(h.hs[32w2].f1) {
            32w0: start3;
            default: accept;
        }
    }
    state start3 {
        transition stateOutOfBound;
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
        b.emit<hdr>(h.hs[0]);
        b.emit<hdr>(h.hs[1]);
        b.emit<hdr>(h.hs[2]);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.set_port") action set_port() {
        sm.egress_spec = (bit<9>)m.v;
    }
    @name("ingress.t") table t_0 {
        actions = {
            set_port();
        }
        const default_action = set_port();
    }
    apply {
        t_0.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
