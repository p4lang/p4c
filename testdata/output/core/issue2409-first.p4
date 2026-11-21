#include <core.p4>

parser p(out bit<32> z) {
    @name("b") bool b_0;
    @name("x") bit<32> x_0;
    @name("w") bit<32> w_0;
    state start {
        b_0 = true;
        transition select(b_0) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        x_0 = 32w1;
        z = x_0;
        transition start_join;
    }
    state start_false {
        w_0 = 32w2;
        z = w_0;
        transition start_join;
    }
    state start_join {
        transition accept;
    }
}

parser _p(out bit<32> z);
package top(_p _pa);
top(p()) main;
