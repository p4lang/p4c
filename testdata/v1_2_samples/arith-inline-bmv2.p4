#include "core.p4"
#include "v1model.p4"

header hdr {
    bit<32> a;
    bit<32> b;
    bit<64> c;
}

control compute(inout hdr h) {
    action add()
    { h.c = h.a + h.b; }
    table t() {
        actions = { add; }
        const default_action = add;
    }
    apply { t.apply(); }
}

#include "arith-inline-skeleton.p4"
