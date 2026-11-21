#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

bit<16> give_val() {
    return 16w1;
}
ethernet_t give_hdr() {
    return (ethernet_t){dst_addr = 48w1,src_addr = 48w1,eth_type = 16w1};
    bit<16> dummy = 16w1;
    return (ethernet_t){dst_addr = 48w1,src_addr = 48w1,eth_type = dummy};
}
control ingress(inout Headers h) {
    Headers foo = (Headers){eth_hdr = (ethernet_t){dst_addr = 48w1,src_addr = 48w1,eth_type = give_val()}};
    apply {
        give_hdr();
        foo.eth_hdr.eth_type[3:0] = 4w1;
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
