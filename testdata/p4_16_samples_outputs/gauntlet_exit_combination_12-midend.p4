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
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_action") action do_action() {
        if (h.eth_hdr.src_addr == 48w1) {
            hasExited = true;
        } else {
            h.eth_hdr.src_addr = 48w1;
        }
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.eth_type: exact @name("tyhSfv");
        }
        actions = {
            do_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_exit_combination_12l42() {
        h.eth_hdr.eth_type = 16w0xdead;
    }
    @hidden action act() {
        hasExited = false;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_gauntlet_exit_combination_12l42 {
        actions = {
            gauntlet_exit_combination_12l42();
        }
        const default_action = gauntlet_exit_combination_12l42();
    }
    apply {
        tbl_act.apply();
        switch (simple_table_0.apply().action_run) {
            do_action: {
                if (hasExited) {
                    ;
                } else {
                    tbl_gauntlet_exit_combination_12l42.apply();
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
