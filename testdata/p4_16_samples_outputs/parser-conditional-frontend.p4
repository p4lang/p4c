#include <core.p4>

parser p(out bit<32> b) {
    bit<32> a_0;
    bit<32> tmp;
    bit<32> tmp_0;
    bit<32> tmp_1;
    state start {
        a_0 = 32w1;
        transition select(a_0 == 32w0) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        tmp = 32w2;
        transition start_join;
    }
    state start_false {
        tmp = 32w3;
        transition start_join;
    }
    state start_join {
        b = tmp;
        b = b + 32w1;
        transition select(a_0 > 32w0) {
            true: start_true_0;
            false: start_false_0;
        }
    }
    state start_true_0 {
        transition select(a_0 > 32w1) {
            true: start_true_0_true;
            false: start_true_0_false;
        }
    }
    state start_true_0_true {
        tmp_1 = b + 32w1;
        transition start_true_0_join;
    }
    state start_true_0_false {
        tmp_1 = b + 32w2;
        transition start_true_0_join;
    }
    state start_true_0_join {
        tmp_0 = tmp_1;
        transition start_join_0;
    }
    state start_false_0 {
        tmp_0 = b + 32w3;
        transition start_join_0;
    }
    state start_join_0 {
        b = tmp_0;
        transition accept;
    }
}

parser proto(out bit<32> b);
package top(proto _p);
top(p()) main;

