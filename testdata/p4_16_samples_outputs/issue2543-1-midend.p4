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
    @hidden action act() {
        retval_0.setValid();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
