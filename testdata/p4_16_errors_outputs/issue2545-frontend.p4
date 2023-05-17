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
    @name("ingress.tmp") Headers tmp_0;
    apply {
        tmp_0 = h;
        call_extern(tmp_0);
        h = tmp_0;
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
