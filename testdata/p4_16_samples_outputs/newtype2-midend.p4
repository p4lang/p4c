#include <core.p4>

struct M {
    bit<9> e;
    bit<9> es;
}

control Ingress(inout M sm);
package V1Switch(Ingress ig);
control FabricIngress(inout M sm) {
    @hidden action newtype2l16() {
        sm.es = (bit<9>)sm.e;
    }
    @hidden table tbl_newtype2l16 {
        actions = {
            newtype2l16();
        }
        const default_action = newtype2l16();
    }
    apply {
        tbl_newtype2l16.apply();
    }
}

V1Switch(FabricIngress()) main;
