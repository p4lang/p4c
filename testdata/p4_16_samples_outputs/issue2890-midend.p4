#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header E {
    bit<16> e;
}

header H {
    bit<16> a;
}

struct Headers {
    E e;
    H h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<E>(hdr.e);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.x") H x_0;
    @hidden action issue2890l32() {
        x_0.setInvalid();
        x_0 = h.h;
        x_0.a = 16w2;
        h.e.e = 16w2;
    }
    @hidden table tbl_issue2890l32 {
        actions = {
            issue2890l32();
        }
        const default_action = issue2890l32();
    }
    apply {
        tbl_issue2890l32.apply();
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

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<E>(h.e);
        pkt.emit<H>(h.h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
