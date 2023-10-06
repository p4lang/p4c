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
    @name("ingress.tmp") bool tmp;
    @name("ingress.tmp_0") bool tmp_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_action") action simple_action() {
        h.eth_hdr.dst_addr = 48w2;
    }
    @name("ingress.exit_action") action exit_action() {
        hasExited = true;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name");
        }
        actions = {
            simple_action();
            exit_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action act() {
        tmp = true;
    }
    @hidden action act_0() {
        tmp = false;
    }
    @hidden action act_1() {
        hasExited = false;
    }
    @hidden action gauntlet_table_call_in_expressionbmv2l48() {
        tmp_0 = h.eth_hdr.src_addr == h.eth_hdr.dst_addr;
    }
    @hidden action gauntlet_table_call_in_expressionbmv2l48_0() {
        tmp_0 = false;
    }
    @hidden action gauntlet_table_call_in_expressionbmv2l49() {
        h.eth_hdr.src_addr = 48w2;
    }
    @hidden table tbl_act {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_gauntlet_table_call_in_expressionbmv2l48 {
        actions = {
            gauntlet_table_call_in_expressionbmv2l48();
        }
        const default_action = gauntlet_table_call_in_expressionbmv2l48();
    }
    @hidden table tbl_gauntlet_table_call_in_expressionbmv2l48_0 {
        actions = {
            gauntlet_table_call_in_expressionbmv2l48_0();
        }
        const default_action = gauntlet_table_call_in_expressionbmv2l48_0();
    }
    @hidden table tbl_gauntlet_table_call_in_expressionbmv2l49 {
        actions = {
            gauntlet_table_call_in_expressionbmv2l49();
        }
        const default_action = gauntlet_table_call_in_expressionbmv2l49();
    }
    apply {
        tbl_act.apply();
        if (simple_table_0.apply().hit) {
            tbl_act_0.apply();
        } else {
            tbl_act_1.apply();
        }
        if (hasExited) {
            ;
        } else {
            if (tmp) {
                tbl_gauntlet_table_call_in_expressionbmv2l48.apply();
            } else {
                tbl_gauntlet_table_call_in_expressionbmv2l48_0.apply();
            }
            if (tmp_0) {
                tbl_gauntlet_table_call_in_expressionbmv2l49.apply();
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
