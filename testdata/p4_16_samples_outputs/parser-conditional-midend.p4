#include <core.p4>

parser p(out bit<32> b) {
    bit<32> a;
    bit<32> tmp_2;
    bit<32> tmp_3;
    bit<32> tmp_4;
    state start {
        a = 32w1;
        transition select((bit<1>)(a == 32w0)) {
            1w1: start_true;
            1w0: start_false;
            default: noMatch;
        }
    }
    state start_true {
        tmp_2 = 32w2;
        transition start_join;
    }
    state start_false {
        tmp_2 = 32w3;
        transition start_join;
    }
    state start_join {
        b = tmp_2;
        b = b + 32w1;
        transition select((bit<1>)(a > 32w0)) {
            1w1: start_true_0;
            1w0: start_false_0;
            default: noMatch;
        }
    }
    state start_true_0 {
        transition select((bit<1>)(a > 32w1)) {
            1w1: start_true_0_true;
            1w0: start_true_0_false;
            default: noMatch;
        }
    }
    state start_true_0_true {
        tmp_4 = b + 32w1;
        transition start_true_0_join;
    }
    state start_true_0_false {
        tmp_4 = b + 32w2;
        transition start_true_0_join;
    }
    state start_true_0_join {
        tmp_3 = tmp_4;
        transition start_join_0;
    }
    state start_false_0 {
        tmp_3 = b + 32w3;
        transition start_join_0;
    }
    state start_join_0 {
        b = tmp_3;
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

parser proto(out bit<32> b);
package top(proto _p);
top(p()) main;

