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
    H[2]       h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<H>(hdr.h.next);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.dummy_val") bit<8> dummy_val_0;
    bit<32> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_action") action simple_action() {
        h.h[0].a = dummy_val_0;
    }
    @name("ingress.simple_action") action simple_action_1() {
        h.h[0].a = dummy_val_0;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("IymcAg");
        }
        actions = {
            simple_action();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_index_2bmv2l34() {
        dummy_val_0 = h.h[0].a;
    }
    @hidden action gauntlet_index_2bmv2l40() {
        dummy_val_0 = 8w255;
        key_0 = 32w1;
    }
    @hidden table tbl_gauntlet_index_2bmv2l34 {
        actions = {
            gauntlet_index_2bmv2l34();
        }
        const default_action = gauntlet_index_2bmv2l34();
    }
    @hidden table tbl_simple_action {
        actions = {
            simple_action_1();
        }
        const default_action = simple_action_1();
    }
    @hidden table tbl_gauntlet_index_2bmv2l40 {
        actions = {
            gauntlet_index_2bmv2l40();
        }
        const default_action = gauntlet_index_2bmv2l40();
    }
    apply {
        tbl_gauntlet_index_2bmv2l34.apply();
        tbl_simple_action.apply();
        tbl_gauntlet_index_2bmv2l40.apply();
        simple_table_0.apply();
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
        pkt.emit<H>(h.h[0]);
        pkt.emit<H>(h.h[1]);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
