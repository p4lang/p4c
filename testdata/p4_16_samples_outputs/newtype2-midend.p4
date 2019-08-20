#include <core.p4>

typedef bit<9> PortIdUInt_t;
typedef bit<9> PortId_t;
struct M {
    PortId_t     e;
    PortIdUInt_t es;
}

control Ingress(inout M sm);
package V1Switch(Ingress ig);
control FabricIngress(inout M sm) {
    @hidden action newtype2l16() {
        sm.es = (PortIdUInt_t)sm.e;
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

