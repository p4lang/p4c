#include <core.p4>

parser p(out bit<32> v) {
    state start {
        v = 32w1;
        transition accept;
    }
}

parser p1(out bit<32> v) {
    state start {
        v = 32w2;
        transition reject;
    }
}

control c(out bit<32> v) {
    apply {
        v = 32w1;
        v = v + 32w2;
        v = v + 32w3;
        v = v + 32w20;
    }
}

parser _p(out bit<32> v);
control _c(out bit<32> v);
package top(_p _p0, _p p1, _c _c0);
top(p(), p1(), c()) main;
