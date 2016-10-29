#include <core.p4>
#include <v1model.p4>

control C(bit<1> meta) {
    apply {
        if (1w0x0 == 1w0) 
            digest<bit<1>>(32w0, meta);
    }
}

