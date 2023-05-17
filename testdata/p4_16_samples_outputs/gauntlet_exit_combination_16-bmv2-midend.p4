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
    @name("ingress.hasReturned") bool hasReturned;
    bit<8> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.dummy_action") action dummy_action() {
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("QIqvRY");
        }
        actions = {
            dummy_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_exit_combination_16bmv2l43() {
        h.eth_hdr.eth_type = 16w1;
        hasReturned = true;
    }
    @hidden action gauntlet_exit_combination_16bmv2l32() {
        hasExited = false;
        hasReturned = false;
        key_0 = 8w255;
    }
    @hidden action gauntlet_exit_combination_16bmv2l48() {
        hasExited = true;
    }
    @hidden table tbl_gauntlet_exit_combination_16bmv2l32 {
        actions = {
            gauntlet_exit_combination_16bmv2l32();
        }
        const default_action = gauntlet_exit_combination_16bmv2l32();
    }
    @hidden table tbl_gauntlet_exit_combination_16bmv2l43 {
        actions = {
            gauntlet_exit_combination_16bmv2l43();
        }
        const default_action = gauntlet_exit_combination_16bmv2l43();
    }
    @hidden table tbl_gauntlet_exit_combination_16bmv2l48 {
        actions = {
            gauntlet_exit_combination_16bmv2l48();
        }
        const default_action = gauntlet_exit_combination_16bmv2l48();
    }
    apply {
        tbl_gauntlet_exit_combination_16bmv2l32.apply();
        switch (simple_table_0.apply().action_run) {
            dummy_action: {
                tbl_gauntlet_exit_combination_16bmv2l43.apply();
            }
            default: {
            }
        }
        if (hasReturned) {
            ;
        } else {
            tbl_gauntlet_exit_combination_16bmv2l48.apply();
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
