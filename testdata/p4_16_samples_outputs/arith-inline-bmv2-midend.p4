#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header hdr {
    bit<32> a;
    bit<32> b;
    bit<64> c;
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
    @name("ingress.c.add") action c_add_0() {
        h.h.c = (bit<64>)(h.h.a + h.h.b);
    }
    @name("ingress.c.t") table c_t {
        actions = {
            c_add_0();
        }
        const default_action = c_add_0();
    }
    @hidden action arithinlineskeleton51() {
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_arithinlineskeleton51 {
        actions = {
            arithinlineskeleton51();
        }
        const default_action = arithinlineskeleton51();
    }
    apply {
        c_t.apply();
        tbl_arithinlineskeleton51.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

