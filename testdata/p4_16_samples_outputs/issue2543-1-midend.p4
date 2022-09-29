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
    @name("ingress.retval_0") ethernet_t retval_0;
    @hidden action issue25431l17() {
        retval_0.setValid();
        retval_0.dst_addr = 48w1;
        retval_0.src_addr = 48w1;
        retval_0.eth_type = 16w1;
    }
    @hidden table tbl_issue25431l17 {
        actions = {
            issue25431l17();
        }
        const default_action = issue25431l17();
    }
    apply {
        tbl_issue25431l17.apply();
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;

