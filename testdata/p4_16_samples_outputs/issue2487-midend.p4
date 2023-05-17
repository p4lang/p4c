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
}

control ingress(inout Headers h) {
    @name("ingress.tmp") ethernet_t tmp;
    @hidden action issue2487l19() {
        tmp.setValid();
        tmp.dst_addr = 48w1;
        tmp.src_addr = 48w1;
        tmp.eth_type = 16w1;
    }
    @hidden action issue2487l19_0() {
        tmp.setValid();
        tmp.dst_addr = 48w2;
        tmp.src_addr = 48w2;
        tmp.eth_type = 16w2;
    }
    @hidden action issue2487l19_1() {
        h.eth_hdr = tmp;
    }
    @hidden table tbl_issue2487l19 {
        actions = {
            issue2487l19();
        }
        const default_action = issue2487l19();
    }
    @hidden table tbl_issue2487l19_0 {
        actions = {
            issue2487l19_0();
        }
        const default_action = issue2487l19_0();
    }
    @hidden table tbl_issue2487l19_1 {
        actions = {
            issue2487l19_1();
        }
        const default_action = issue2487l19_1();
    }
    apply {
        if (h.eth_hdr.eth_type == 16w1) {
            tbl_issue2487l19.apply();
        } else {
            tbl_issue2487l19_0.apply();
        }
        tbl_issue2487l19_1.apply();
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
