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
    @name("ingress.hasReturned") bool hasReturned;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.call_action") action call_action() {
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
        }
        actions = {
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action gauntlet_nested_switchbmv2l47() {
        h.eth_hdr.eth_type = 16w1;
    }
    @hidden action gauntlet_nested_switchbmv2l38() {
        hasReturned = true;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_call_action {
        actions = {
            call_action();
        }
        const default_action = call_action();
    }
    @hidden table tbl_gauntlet_nested_switchbmv2l38 {
        actions = {
            gauntlet_nested_switchbmv2l38();
        }
        const default_action = gauntlet_nested_switchbmv2l38();
    }
    @hidden table tbl_gauntlet_nested_switchbmv2l47 {
        actions = {
            gauntlet_nested_switchbmv2l47();
        }
        const default_action = gauntlet_nested_switchbmv2l47();
    }
    apply {
        tbl_act.apply();
        tbl_call_action.apply();
        tbl_gauntlet_nested_switchbmv2l38.apply();
        if (hasReturned) {
            ;
        } else {
            tbl_gauntlet_nested_switchbmv2l47.apply();
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

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

