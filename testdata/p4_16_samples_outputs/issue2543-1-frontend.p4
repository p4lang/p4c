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
    apply {
        {
            @name("ingress.hasReturned") bool hasReturned = false;
            @name("ingress.retval") bit<16> retval;
            hasReturned = true;
            retval = 16w1;
        }
        {
            @name("ingress.hasReturned_0") bool hasReturned_0 = false;
            @name("ingress.retval_0") ethernet_t retval_0;
            hasReturned_0 = true;
            retval_0.setValid();
            retval_0 = (ethernet_t){dst_addr = 48w1,src_addr = 48w1,eth_type = 16w1};
        }
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;

