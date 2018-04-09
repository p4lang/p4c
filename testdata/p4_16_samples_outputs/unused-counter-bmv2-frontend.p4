#include <core.p4>
#include <v1model.p4>

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
    @name("ingress.c") direct_counter(CounterType.packets) c;
    @name("ingress.c1") direct_counter(CounterType.packets) c1;
    @name("ingress.my_action") action my_action_0(bit<9> a) {
        sm.egress_spec = a;
    }
    @name("ingress.t") table t {
        actions = {
            my_action_0();
        }
        const default_action = my_action_0(9w0);
        counters = c;
    }
    apply {
        t.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

