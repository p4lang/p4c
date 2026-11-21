#include <core.p4>

control c(in bit<1> b) {
    table t {
        key = {
            b: noSuchMatch;
        }
        actions = {
            NoAction;
        }
    }
    apply {
    }
}

