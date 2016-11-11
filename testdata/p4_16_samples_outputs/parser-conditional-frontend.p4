parser p(out bit<32> b) {
    bit<32> a_0;
    bool tmp;
    bit<32> tmp_0;
    bit<32> tmp_1;
    bool tmp_2;
    bit<32> tmp_3;
    bool tmp_4;
    bit<32> tmp_5;
    bit<32> tmp_6;
    bit<32> tmp_7;
    bit<32> tmp_8;
    state start {
        a_0 = 32w1;
        tmp = a_0 == 32w0;
        transition select(tmp) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        tmp_0 = 32w2;
        transition start_join;
    }
    state start_false {
        tmp_0 = 32w3;
        transition start_join;
    }
    state start_join {
        b = tmp_0;
        tmp_1 = b + 32w1;
        b = tmp_1;
        tmp_2 = a_0 > 32w0;
        transition select(tmp_2) {
            true: start_true_0;
            false: start_false_0;
        }
    }
    state start_true_0 {
        tmp_4 = a_0 > 32w1;
        transition select(tmp_4) {
            true: start_true_0_true;
            false: start_true_0_false;
        }
    }
    state start_true_0_true {
        tmp_6 = b + 32w1;
        tmp_5 = tmp_6;
        transition start_true_0_join;
    }
    state start_true_0_false {
        tmp_7 = b + 32w2;
        tmp_5 = tmp_7;
        transition start_true_0_join;
    }
    state start_true_0_join {
        tmp_3 = tmp_5;
        transition start_join_0;
    }
    state start_false_0 {
        tmp_8 = b + 32w3;
        tmp_3 = tmp_8;
        transition start_join_0;
    }
    state start_join_0 {
        b = tmp_3;
        transition accept;
    }
}

parser proto(out bit<32> b);
package top(proto _p);
top(p()) main;
