#include <core.p4>
#include <v1model.p4>

header hdr {
  bit<32> a;
}

control compute(inout hdr h) {
    bit<8> n = 0;
    apply {
        if (!h.isValid()) {
            return;
        }

        if (n > 0) {
            h.setValid();
        }
    }
}

#include "arith-inline-skeleton.p4"
