#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") ethernet_t tmp;
    @hidden action issue25432l22() {
        tmp.setValid();
        tmp.dst_addr = 48w1;
        tmp.src_addr = 48w1;
        tmp.eth_type = 16w9;
        h.eth_hdr = tmp;
    }
    @hidden table tbl_issue25432l22 {
        actions = {
            issue25432l22();
        }
        const default_action = issue25432l22();
    }
    apply {
        tbl_issue25432l22.apply();
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
