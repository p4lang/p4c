#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

control C(bit<1> meta) {
    apply {
        digest<bit<1>>(32w0, meta);
    }
}

