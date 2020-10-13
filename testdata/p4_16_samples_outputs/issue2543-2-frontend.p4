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
            bool hasReturned = false;
            Headers retval;
            ethernet_t tmp;
            bit<48> tmp_0;
            bit<48> tmp_1;
            bit<16> tmp_2;
            bit<16> tmp_3;
            tmp_0 = 48w1;
            tmp_1 = 48w1;
            {
                bool hasReturned_0 = false;
                bit<16> retval_0;
                hasReturned_0 = true;
                retval_0 = 16w9;
                tmp_3 = retval_0;
            }
            tmp_2 = tmp_3;
            tmp.setValid();
            tmp = (ethernet_t){dst_addr = tmp_0,src_addr = tmp_1,eth_type = tmp_2};
            hasReturned = true;
            retval = (Headers){eth_hdr = tmp};
            h = retval;
        }
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;

