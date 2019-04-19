#include <core.p4>
#include <v1model.p4>

control C(bit<1> meta) {
    apply {
        if (meta & 0x0 == 0) {
            digest(0, meta);
        }
    }
}

