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
    @hidden action issue25461l39() {
        tmp = true;
    }
    @hidden action issue25461l39_0() {
        tmp = false;
    }
    @hidden action issue25461l39_1() {
        tmp_0 = 8w1;
    }
    @hidden action issue25461l39_2() {
        tmp_0 = 8w2;
    }
    @hidden action issue25461l46() {
        h.eth_hdr.dst_addr = 48w1;
    }
    @hidden table tbl_issue25461l39 {
        actions = {
            issue25461l39();
        }
        const default_action = issue25461l39();
    }
    @hidden table tbl_issue25461l39_0 {
        actions = {
            issue25461l39_0();
        }
        const default_action = issue25461l39_0();
    }
    @hidden table tbl_issue25461l39_1 {
        actions = {
            issue25461l39_1();
        }
        const default_action = issue25461l39_1();
    }
    @hidden table tbl_issue25461l39_2 {
        actions = {
            issue25461l39_2();
        }
        const default_action = issue25461l39_2();
    }
    @hidden table tbl_issue25461l46 {
        actions = {
            issue25461l46();
        }
        const default_action = issue25461l46();
    }
    apply {
        if (simple_table.apply().hit) {
            tbl_issue25461l39.apply();
        } else {
            tbl_issue25461l39_0.apply();
        }
        if (tmp) {
            tbl_issue25461l39_1.apply();
        } else {
            tbl_issue25461l39_2.apply();
        }
        if (simple_table_0.apply().hit) {
            tbl_issue25461l46.apply();
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;
