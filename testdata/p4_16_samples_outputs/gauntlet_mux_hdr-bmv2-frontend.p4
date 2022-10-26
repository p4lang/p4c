#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<32> a;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.retval") bit<32> retval;
    @name("ingress.tmp1") H[2] tmp1_0;
    @name("ingress.tmp2") H[2] tmp2_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_action") action simple_action() {
        tmp1_0[0].setInvalid();
        tmp1_0[1].setInvalid();
        tmp2_0[0].setInvalid();
        tmp2_0[1].setInvalid();
        if (tmp2_0[0].a <= 32w3) {
            tmp1_0[0] = tmp2_0[1];
        }
        retval = tmp1_0[0].a;
        h.h.a = retval;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            sm.egress_spec: exact @name("key");
        }
        actions = {
            simple_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        simple_table_0.apply();
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
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

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
