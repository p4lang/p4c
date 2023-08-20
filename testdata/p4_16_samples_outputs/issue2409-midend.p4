#include <core.p4>

parser p(out bit<32> z) {
    state start {
        transition start_true;
    }
    state start_true {
        z = 32w1;
        transition start_join;
    }
    state start_join {
        transition accept;
    }
}

parser _p(out bit<32> z);
package top(_p _pa);
top(p()) main;
