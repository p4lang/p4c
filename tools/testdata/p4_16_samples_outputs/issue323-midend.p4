#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<32> f;
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
    @name("ingress.my_a") action my_a() {
        h.h.f = 32w0;
    }
    @name("ingress.my_a") action my_a_2() {
        h.h.f = 32w1;
    }
    @hidden table tbl_my_a {
        actions = {
            my_a();
        }
        const default_action = my_a();
    }
    @hidden table tbl_my_a_0 {
        actions = {
            my_a_2();
        }
        const default_action = my_a_2();
    }
    apply {
        tbl_my_a.apply();
        tbl_my_a_0.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

