#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp_key") bit<128> tmp_key_0;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.val") bit<8> val_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_action") action do_action() {
        val_0 = h.h.a;
        hasReturned = false;
        if (val_0 > 8w10) {
            val_0 = 8w2;
            hasReturned = true;
        } else {
            val_0 = 8w3;
        }
        h.h.a = val_0;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            tmp_key_0: exact @name("bKiScA");
        }
        actions = {
            do_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        tmp_key_0 = 128w2;
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
