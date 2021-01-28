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
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.tmp") bit<16> tmp;
    @name("ingress.tmp_0") bool tmp_0;
    @name("ingress.tmp_1") bit<16> tmp_1;
    @name("ingress.exit_action") action exit_action() {
        exit;
    }
    @name("ingress.tbl") table tbl_0 {
        key = {
            h.eth_hdr.src_addr: exact @name("fbgPij") ;
        }
        actions = {
            exit_action();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        if (h.eth_hdr.eth_type == 16w4) {
            tmp_0 = tbl_0.apply().hit;
            if (tmp_0) {
                tmp_1 = 16w1;
            } else {
                tmp_1 = 16w2;
            }
            tmp = tmp_1;
        } else {
            tmp = 16w3;
        }
        h.eth_hdr.eth_type = tmp;
        exit;
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

