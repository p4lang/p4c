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

control ingress(inout Headers h) {
    action do_action() {
        if (h.eth_hdr.src_addr == 1) {
            exit;
        } else {
            h.eth_hdr.src_addr = 48w1;
        }
    }
    table simple_table {
        key = {
            h.eth_hdr.eth_type: exact @name("tyhSfv") ;
        }
        actions = {
            do_action();
        }
    }
    apply {
        switch (simple_table.apply().action_run) {
            do_action: {
                h.eth_hdr.eth_type = 0xDEAD;
            }
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

