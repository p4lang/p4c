#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    @name("ingress.c.hasReturned") bool c_hasReturned;
    @hidden action issue1386l12() {
        c_hasReturned = true;
    }
    @hidden action act() {
        c_hasReturned = false;
    }
    @hidden action arithinlineskeleton51() {
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_issue1386l12 {
        actions = {
            issue1386l12();
        }
        const default_action = issue1386l12();
    }
    @hidden table tbl_arithinlineskeleton51 {
        actions = {
            arithinlineskeleton51();
        }
        const default_action = arithinlineskeleton51();
    }
    apply {
        tbl_act.apply();
        if (h.h.isValid()) {
            ;
        } else {
            tbl_issue1386l12.apply();
        }
        tbl_arithinlineskeleton51.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
