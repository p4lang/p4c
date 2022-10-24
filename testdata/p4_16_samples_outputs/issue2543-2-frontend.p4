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
    @name("ingress.retval_0") Headers retval;
    @name("ingress.tmp") ethernet_t tmp;
    @name("ingress.tmp_0") bit<48> tmp_0;
    @name("ingress.tmp_1") bit<48> tmp_1;
    @name("ingress.tmp_2") bit<16> tmp_2;
    @name("ingress.tmp_3") bit<16> tmp_3;
    @name("ingress.retval") bit<16> retval_0;
    apply {
        tmp_0 = 48w1;
        tmp_1 = 48w1;
        retval_0 = 16w9;
        tmp_3 = retval_0;
        tmp_2 = tmp_3;
        tmp.setValid();
        tmp = (ethernet_t){dst_addr = tmp_0,src_addr = tmp_1,eth_type = tmp_2};
        retval = (Headers){eth_hdr = tmp};
        h = retval;
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
