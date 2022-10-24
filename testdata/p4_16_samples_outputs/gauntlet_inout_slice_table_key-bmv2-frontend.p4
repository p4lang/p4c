#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp_val") bit<8> tmp_val_0;
    @name("ingress.tmp") bit<4> tmp;
    @name("ingress.dummy") bit<4> dummy_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_action") action simple_action() {
        dummy_0 = tmp;
        tmp = dummy_0;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            tmp_val_0: exact @name("dummy");
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        tmp_val_0 = 8w1;
        tmp = tmp_val_0[7:4];
        simple_action();
        tmp_val_0[7:4] = tmp;
        if (simple_table_0.apply().hit) {
            h.eth_hdr.eth_type = 16w1;
        }
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
        pkt.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
