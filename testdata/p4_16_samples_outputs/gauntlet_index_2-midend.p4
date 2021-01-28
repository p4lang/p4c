#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

struct Headers {
    ethernet_t eth_hdr;
    H[2]       h;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<H>(hdr.h.next);
        transition accept;
    }
}

control ingress(inout Headers h) {
    @name("ingress.dummy_val") bit<8> dummy_val_0;
    bit<32> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.simple_action") action simple_action() {
        h.h[0].a = dummy_val_0;
    }
    @name("ingress.simple_action") action simple_action_2() {
        h.h[0].a = dummy_val_0;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("IymcAg") ;
        }
        actions = {
            simple_action();
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action gauntlet_index_2l30() {
        dummy_val_0 = h.h[0].a;
    }
    @hidden action gauntlet_index_2l36() {
        dummy_val_0 = 8w255;
        key_0 = 32w1;
    }
    @hidden table tbl_gauntlet_index_2l30 {
        actions = {
            gauntlet_index_2l30();
        }
        const default_action = gauntlet_index_2l30();
    }
    @hidden table tbl_simple_action {
        actions = {
            simple_action_2();
        }
        const default_action = simple_action_2();
    }
    @hidden table tbl_gauntlet_index_2l36 {
        actions = {
            gauntlet_index_2l36();
        }
        const default_action = gauntlet_index_2l36();
    }
    apply {
        tbl_gauntlet_index_2l30.apply();
        tbl_simple_action.apply();
        tbl_gauntlet_index_2l36.apply();
        simple_table_0.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

