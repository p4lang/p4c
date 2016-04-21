#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

header hdr {
    bit<32> a;
    bit<8>  b;
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
        bool hasExited = false;
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
        bool hasExited_0 = false;
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        bool hasExited_1 = false;
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        bool hasExited_2 = false;
        b.emit(h.h);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action shift() {
        h.h.c = (bit<64>)(h.h.a >> h.h.b);
        sm.egress_spec = 9w0;
    }
    table t() {
        actions = {
            shift;
        }
        const default_action = shift;
    }

    apply {
        bool hasExited_3 = false;
        t.apply();
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
