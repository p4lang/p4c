#include "core.p4"
#include "v1model.p4"

header hdr {
    bit<32> a;
    bit<32> b;
}

control compute(inout hdr h) {
    action add(bit<32> data)
    { h.b = h.a + data; }
    table t() {
        actions = { add; }
        const default_action = add(10);
    }
    apply { t.apply(); }
}

#include "arith-inline-skeleton.p4"
