#include <core.p4>

control MyC(bit<8> t) {
    table t {
        key = {
            t: exact @name("t") ;
        }
        actions = {
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
    }
}

