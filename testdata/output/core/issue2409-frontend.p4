#include <core.p4>

parser p(out bit<32> z) {
    @name("p.b") bool b;
    @name("p.x") bit<32> x;
    @name("p.w") bit<32> w;
    state start {
        b = true;
        transition select(b) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        x = 32w1;
        z = x;
        transition start_join;
    }
    state start_false {
        w = 32w2;
        z = w;
        transition start_join;
    }
    state start_join {
        transition accept;
    }
}

parser _p(out bit<32> z);
package top(_p _pa);
top(p()) main;
