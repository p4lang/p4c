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
    apply {
        h.eth_hdr = h.eth_hdr.eth_type == 1 ? {48w1, 48w1, 16w1} : {48w2, 48w2, 16w2};
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
