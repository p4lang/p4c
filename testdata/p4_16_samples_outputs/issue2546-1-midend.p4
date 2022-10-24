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
    @name("ingress.tmp") bool tmp;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("ingress.simple_table_1") table simple_table {
        key = {
            h.eth_hdr.eth_type: exact @name("KOXpQP");
        }
        actions = {
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("ingress.simple_table_2") table simple_table_0 {
        key = {
            tmp_0: exact @name("key");
        }
        actions = {
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @hidden action act() {
        tmp = true;
    }
    @hidden action act_0() {
        tmp = false;
    }
    @hidden action issue25461l33() {
        tmp_0 = 8w1;
    }
    @hidden action issue25461l33_0() {
        tmp_0 = 8w2;
    }
    @hidden action issue25461l40() {
        h.eth_hdr.dst_addr = 48w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_issue25461l33 {
        actions = {
            issue25461l33();
        }
        const default_action = issue25461l33();
    }
    @hidden table tbl_issue25461l33_0 {
        actions = {
            issue25461l33_0();
        }
        const default_action = issue25461l33_0();
    }
    @hidden table tbl_issue25461l40 {
        actions = {
            issue25461l40();
        }
        const default_action = issue25461l40();
    }
    apply {
        if (simple_table.apply().hit) {
            tbl_act.apply();
        } else {
            tbl_act_0.apply();
        }
        if (tmp) {
            tbl_issue25461l33.apply();
        } else {
            tbl_issue25461l33_0.apply();
        }
        if (simple_table_0.apply().hit) {
            tbl_issue25461l40.apply();
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;
