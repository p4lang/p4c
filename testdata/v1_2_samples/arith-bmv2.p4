#include "core.p4"
#include "v1model.p4"

header hdr {
    bit<32> a;
    bit<32> b;
    bit<64> c;
}

#include "arith-skeleton.p4"

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action add()
    { h.h.c = h.h.a + h.h.b; sm.egress_spec = 0; }
    table t() {
        actions = { add; }
        const default_action = add;
    }
    apply { t.apply(); }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
