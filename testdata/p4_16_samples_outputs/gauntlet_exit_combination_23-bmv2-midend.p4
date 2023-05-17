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
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_action") action simple_action() {
        h.eth_hdr.dst_addr = 48w1;
        hasExited = true;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.eth_type: exact @name("key");
        }
        actions = {
            simple_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_exit_combination_23bmv2l43() {
        hasExited = true;
    }
    @hidden action act() {
        hasExited = false;
    }
    @hidden action gauntlet_exit_combination_23bmv2l46() {
        h.eth_hdr.dst_addr = h.eth_hdr.src_addr + h.eth_hdr.dst_addr;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_gauntlet_exit_combination_23bmv2l43 {
        actions = {
            gauntlet_exit_combination_23bmv2l43();
        }
        const default_action = gauntlet_exit_combination_23bmv2l43();
    }
    @hidden table tbl_gauntlet_exit_combination_23bmv2l46 {
        actions = {
            gauntlet_exit_combination_23bmv2l46();
        }
        const default_action = gauntlet_exit_combination_23bmv2l46();
    }
    apply {
        tbl_act.apply();
        switch (simple_table_0.apply().action_run) {
            simple_action: {
                if (hasExited) {
                    ;
                } else {
                    tbl_gauntlet_exit_combination_23bmv2l43.apply();
                }
            }
            default: {
            }
        }
        if (hasExited) {
            ;
        } else {
            tbl_gauntlet_exit_combination_23bmv2l46.apply();
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
