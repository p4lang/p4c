#include <core.p4>
header ethernet_t {
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

control SubCtrl(bit<16> eth_type) {
    table dummy {
        key = {
            eth_type: exact @name("dummy_key") ;
        }
        actions = {
        }
    }
    apply {
        dummy.apply();
    }
}

control ingress(inout Headers h) {
    SubCtrl() sub;
    apply {
        sub.apply(16w2);
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
