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
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control SubCtrl() {
        apply {
    }
}

control ingress(inout Headers h) {
    SubCtrl() sub_ctrl_1;
    SubCtrl() sub_ctrl_2;

    action call_action() {
        // if we move this statement out of here it crashes
        sub_ctrl_1.apply();
    }
    apply {
        call_action();
        sub_ctrl_2.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);

package top(Parser p, Ingress ig);
top(p(), ingress()) main;
