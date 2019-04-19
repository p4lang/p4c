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
    @hidden action act() {
        sm.es = (PortIdUInt_t)sm.e;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

V1Switch(FabricIngress()) main;

