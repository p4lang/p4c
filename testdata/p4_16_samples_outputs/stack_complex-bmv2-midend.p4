#include <core.p4>
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
    state start {
        b.extract<hdr>(h.hs.next);
        m.v = h.hs.last.f2;
        m.v = m.v + h.hs.last.f2;
        transition select(h.hs.last.f1) {
            32w0: start;
            default: accept;
        }
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
    @name("ingress.set_port") action set_port_0() {
        sm.egress_spec = (bit<9>)m.v;
    }
    @name("ingress.t") table t {
        actions = {
            set_port_0();
        }
        const default_action = set_port_0();
    }
    apply {
        t.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

