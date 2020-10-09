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
    bit<8> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.simple_table_1") table simple_table {
        key = {
            h.eth_hdr.eth_type: exact @name("KOXpQP") ;
        }
        actions = {
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name("ingress.simple_table_2") table simple_table_0 {
        key = {
            key_0: exact @name("key") ;
        }
        actions = {
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    @hidden action nested_tbl_apply1l39() {
        h.eth_hdr.dst_addr = 48w1;
    }
    @hidden action nested_tbl_apply1l32() {
        key_0 = (simple_table.apply().hit ? 8w1 : 8w2);
    }
    @hidden table tbl_nested_tbl_apply1l32 {
        actions = {
            nested_tbl_apply1l32();
        }
        const default_action = nested_tbl_apply1l32();
    }
    @hidden table tbl_nested_tbl_apply1l39 {
        actions = {
            nested_tbl_apply1l39();
        }
        const default_action = nested_tbl_apply1l39();
    }
    apply {
        tbl_nested_tbl_apply1l32.apply();
        if (simple_table_0.apply().hit) {
            tbl_nested_tbl_apply1l39.apply();
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

