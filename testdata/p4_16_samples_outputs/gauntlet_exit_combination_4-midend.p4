#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h) {
    bool hasExited;
    bit<48> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.exit_action") action exit_action() {
        hasExited = true;
    }
    @name("ingress.exit_action") action exit_action_2() {
        hasExited = true;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("key") ;
        }
        actions = {
            exit_action();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action gauntlet_exit_combination_4l38() {
        h.eth_hdr.eth_type = 16w1;
    }
    @hidden action gauntlet_exit_combination_4l29() {
        hasExited = false;
        key_0 = 48w100;
    }
    @hidden table tbl_gauntlet_exit_combination_4l29 {
        actions = {
            gauntlet_exit_combination_4l29();
        }
        const default_action = gauntlet_exit_combination_4l29();
    }
    @hidden table tbl_gauntlet_exit_combination_4l38 {
        actions = {
            gauntlet_exit_combination_4l38();
        }
        const default_action = gauntlet_exit_combination_4l38();
    }
    @hidden table tbl_exit_action {
        actions = {
            exit_action_2();
        }
        const default_action = exit_action_2();
    }
    apply {
        tbl_gauntlet_exit_combination_4l29.apply();
        switch (simple_table_0.apply().action_run) {
            exit_action: {
                if (!hasExited) {
                    tbl_gauntlet_exit_combination_4l38.apply();
                    tbl_exit_action.apply();
                }
            }
            default: {
            }
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

