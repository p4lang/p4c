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
    bit<32> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.dummy") action dummy() {
    }
    @name("ingress.do_action") action do_action() {
        h.eth_hdr.eth_type = 16w1;
        hasExited = true;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("akSTMF");
        }
        actions = {
            dummy();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_exit_combination_1bmv2l52() {
        hasReturned = true;
    }
    @hidden action gauntlet_exit_combination_1bmv2l39() {
        hasExited = false;
        hasReturned = false;
        key_0 = 32w1;
    }
    @hidden table tbl_gauntlet_exit_combination_1bmv2l39 {
        actions = {
            gauntlet_exit_combination_1bmv2l39();
        }
        const default_action = gauntlet_exit_combination_1bmv2l39();
    }
    @hidden table tbl_gauntlet_exit_combination_1bmv2l52 {
        actions = {
            gauntlet_exit_combination_1bmv2l52();
        }
        const default_action = gauntlet_exit_combination_1bmv2l52();
    }
    @hidden table tbl_do_action {
        actions = {
            do_action();
        }
        const default_action = do_action();
    }
    apply {
        tbl_gauntlet_exit_combination_1bmv2l39.apply();
        switch (simple_table_0.apply().action_run) {
            dummy: {
            }
            default: {
                tbl_gauntlet_exit_combination_1bmv2l52.apply();
            }
        }
        if (hasReturned) {
            ;
        } else {
            tbl_do_action.apply();
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
