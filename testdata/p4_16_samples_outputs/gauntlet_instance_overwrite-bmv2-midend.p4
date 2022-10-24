#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

struct Headers {
    H h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp_1") H tmp;
    @name("ingress.val") bit<8> val_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_action") action do_action() {
        sm.ingress_port = 9w0;
    }
    @name("ingress.BfyXpa") table BfyXpa_0 {
        key = {
            val_0: exact @name("eJPEfW");
        }
        actions = {
            do_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_instance_overwritebmv2l23() {
        tmp.setValid();
        tmp.a = 8w3;
        tmp.b = 8w1;
        tmp.c = 8w1;
        val_0 = 8w3;
    }
    @hidden table tbl_gauntlet_instance_overwritebmv2l23 {
        actions = {
            gauntlet_instance_overwritebmv2l23();
        }
        const default_action = gauntlet_instance_overwritebmv2l23();
    }
    apply {
        tbl_gauntlet_instance_overwritebmv2l23.apply();
        BfyXpa_0.apply();
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
        pkt.emit<H>(h.h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
