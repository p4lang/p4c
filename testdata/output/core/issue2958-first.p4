#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
}

struct G {
    ethernet_t eth;
}

struct H {
    G g;
}

struct Headers {
    ethernet_t eth_hdr;
}

control ingress(inout Headers h) {
    H tmp = (H){g = (G){eth = (ethernet_t){dst_addr = 48w1}}};
    apply {
        tmp.g.eth.dst_addr = 48w1;
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
