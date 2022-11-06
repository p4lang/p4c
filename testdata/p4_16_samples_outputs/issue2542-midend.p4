#include <core.p4>

header ethernet_t {
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

control ingress(inout Headers h) {
    bit<16> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.sub.dummy") table sub_dummy {
        key = {
            key_0: exact @name("dummy_key");
        }
        actions = {
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action issue2542l26() {
        key_0 = 16w2;
    }
    @hidden table tbl_issue2542l26 {
        actions = {
            issue2542l26();
        }
        const default_action = issue2542l26();
    }
    apply {
        tbl_issue2542l26.apply();
        sub_dummy.apply();
    }
}

control Ingress(inout Headers hdr);
package top(Ingress ig);
top(ingress()) main;
