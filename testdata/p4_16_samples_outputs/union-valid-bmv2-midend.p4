#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header Hdr1 {
    bit<32> a;
}

header Hdr2 {
    bit<64> b;
}

struct Headers {
    Hdr1 h1;
    Hdr1 u_h1;
    Hdr2 u_h2;
}

struct Meta {
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract<Hdr1>(h.h1);
        transition select(h.h1.a) {
            32w0: getH1;
            default: getH1;
        }
    }
    state getH1 {
        b.extract<Hdr1>(h.u_h1);
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
        b.emit<Hdr1>(h.h1);
        b.emit<Hdr1>(h.u_h1);
        b.emit<Hdr2>(h.u_h2);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bool key_0;
    @name("ingress.a") action a_1() {
    }
    @name("ingress.t") table t_0 {
        key = {
            key_0: exact @name("h.u.$valid$");
        }
        actions = {
            a_1();
        }
        default_action = a_1();
    }
    @hidden action unionvalidbmv2l76() {
        bit<32> tmp;
        tmp = 32w0;
        if (h.u_h1.isValid()) {
            tmp = tmp + 32w1;
        }
        if (h.u_h2.isValid()) {
            tmp = tmp + 32w1;
        }
        key_0 = tmp == 32w1;
    }
    @hidden table tbl_unionvalidbmv2l76 {
        actions = {
            unionvalidbmv2l76();
        }
        const default_action = unionvalidbmv2l76();
    }
    apply {
        tbl_unionvalidbmv2l76.apply();
        t_0.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
