#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

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
        b.extract(h.h);
        transition accept;
    }
}

control vrfy(in Headers h, inout Meta m, inout standard_metadata_t sm) {
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
        b.emit(h.h);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    hdr h_0;
    @name("c.add") action c_add() {
        h_0.c = (bit<64>)(h_0.a + h_0.b);
    }
    @name("c.t") table c_t() {
        actions = {
            c_add;
        }
        const default_action = c_add;
    }
    action act() {
        h_0 = h.h;
    }
    action act_0() {
        h.h = h_0;
        sm.egress_spec = 9w0;
    }
    table tbl_act() {
        actions = {
            act;
        }
        const default_action = act();
    }
    table tbl_act_0() {
        actions = {
            act_0;
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        c_t.apply();
        tbl_act_0.apply();
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
