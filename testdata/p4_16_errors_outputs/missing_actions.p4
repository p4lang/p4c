#include "/home/cdodd/p4c/p4include/core.p4"

control c(in bit<1> x) {
    table t() {
        key = {
            x: exact;
        }
    }
    apply {
    }
}

