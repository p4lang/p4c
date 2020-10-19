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
    @name("ingress.foo") Headers foo_0;
    @name("ingress.tmp") ethernet_t tmp;
    @name("ingress.tmp_0") bit<48> tmp_0;
    @name("ingress.tmp_1") bit<48> tmp_1;
    @name("ingress.tmp_2") bit<16> tmp_2;
    @name("ingress.tmp_3") bit<16> tmp_3;
    apply {
        tmp_0 = 48w1;
        tmp_1 = 48w1;
        {
            @name("ingress.hasReturned") bool hasReturned = false;
            @name("ingress.retval") bit<16> retval;
            hasReturned = true;
            retval = 16w1;
            tmp_3 = retval;
        }
        tmp_2 = tmp_3;
        tmp.setValid();
        tmp = (ethernet_t){dst_addr = tmp_0,src_addr = tmp_1,eth_type = tmp_2};
        foo_0 = (Headers){eth_hdr = tmp};
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

