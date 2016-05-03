#include "core.p4"
#include "v1model.p4"

header hdr {
    bit<32> a;
    bit<32> b;
    bit<8> c;
}

control compute(inout hdr h) {
    apply {
        if (h.a < h.b)
            h.c = 0;
        else
            h.c = 1;
    }
}

#include "arith-inline-skeleton.p4"
