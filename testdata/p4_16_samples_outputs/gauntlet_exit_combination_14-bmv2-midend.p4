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
    bool hasExited;
    bit<128> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.exit_action") action exit_action() {
        hasExited = true;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("key") ;
        }
        actions = {
            exit_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_exit_combination_14bmv2l41() {
        h.eth_hdr.eth_type = 16w1;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_14bmv2l32() {
        hasExited = false;
        key_0 = 128w1;
    }
    @hidden table tbl_gauntlet_exit_combination_14bmv2l32 {
        actions = {
            gauntlet_exit_combination_14bmv2l32();
        }
        const default_action = gauntlet_exit_combination_14bmv2l32();
    }
    @hidden table tbl_gauntlet_exit_combination_14bmv2l41 {
        actions = {
            gauntlet_exit_combination_14bmv2l41();
        }
        const default_action = gauntlet_exit_combination_14bmv2l41();
    }
    apply {
        tbl_gauntlet_exit_combination_14bmv2l32.apply();
        switch (simple_table_0.apply().action_run) {
            exit_action: {
                if (hasExited) {
                    ;
                } else {
                    tbl_gauntlet_exit_combination_14bmv2l41.apply();
                }
            }
            default: {
            }
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

