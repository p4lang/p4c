#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

extern bit<64> call_extern(inout Headers val);
control ingress(inout Headers h) {
    apply {
        Headers tmp = h;
        call_extern(tmp);
        h = tmp;
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
