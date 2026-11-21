#include <core.p4>

control MyC(bit<8> t) {
    table t {
        key = {
            t: exact;
        }
        actions = {
        }
    }
    apply {
    }
}

