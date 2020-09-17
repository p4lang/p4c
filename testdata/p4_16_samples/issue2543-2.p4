#include <core.p4>
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

bit<16> produce_val() {
    return 16w9;
}

Headers produce_hdr() {
    return { { 1, 1, produce_val() }};
}

control ingress(inout Headers h) {
    apply {
        h = produce_hdr();
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;

