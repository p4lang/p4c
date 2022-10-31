#include <core.p4>

parser p(out bit<32> b) {
    @name("p.a") bit<32> a_0;
    @name("p.tmp") bit<32> tmp;
    @name("p.tmp_0") bit<32> tmp_0;
    @name("p.tmp_1") bit<32> tmp_1;
    state start {
        a_0 = 32w1;
        transition start_0;
    }
    state start_0 {
        transition select(a_0 == 32w0) {
            true: start_0_true;
            false: start_0_false;
        }
    }
    state start_0_true {
        tmp = 32w2;
        transition start_0_join;
    }
    state start_0_false {
        tmp = 32w3;
        transition start_0_join;
    }
    state start_0_join {
        b = tmp;
        b = b + 32w1;
        transition select(a_0 > 32w0) {
            true: start_0_true_0;
            false: start_0_false_0;
        }
    }
    state start_0_true_0 {
        transition select(a_0 > 32w1) {
            true: start_0_true_0_true;
            false: start_0_true_0_false;
        }
    }
    state start_0_true_0_true {
        tmp_1 = b + 32w1;
        transition start_0_true_0_join;
    }
    state start_0_true_0_false {
        tmp_1 = b + 32w2;
        transition start_0_true_0_join;
    }
    state start_0_true_0_join {
        tmp_0 = tmp_1;
        transition start_0_join_0;
    }
    state start_0_false_0 {
        tmp_0 = b + 32w3;
        transition start_0_join_0;
    }
    state start_0_join_0 {
        b = tmp_0;
        transition accept;
    }
}

parser proto(out bit<32> b);
package top(proto _p);
top(p()) main;
