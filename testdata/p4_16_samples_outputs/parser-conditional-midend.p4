parser p() {
    @name("a") bit<32> a;
    @name("b") bit<32> b;
    @name("tmp") bool tmp_9;
    @name("tmp_0") bit<32> tmp_10;
    @name("tmp_1") bit<32> tmp_11;
    @name("tmp_2") bool tmp_12;
    @name("tmp_3") bit<32> tmp_13;
    @name("tmp_4") bool tmp_14;
    @name("tmp_5") bit<32> tmp_15;
    @name("tmp_6") bit<32> tmp_16;
    @name("tmp_7") bit<32> tmp_17;
    @name("tmp_8") bit<32> tmp_18;
    state start {
        a = 32w1;
        tmp_9 = a == 32w0;
        transition select(tmp_9) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        tmp_10 = 32w2;
        transition start_join;
    }
    state start_false {
        tmp_10 = 32w3;
        transition start_join;
    }
    state start_join {
        b = tmp_10;
        tmp_11 = b + 32w1;
        b = tmp_11;
        tmp_12 = a > 32w0;
        transition select(tmp_12) {
            true: start_true_0;
            false: start_false_0;
        }
    }
    state start_true_0 {
        tmp_14 = a > 32w1;
        transition select(tmp_14) {
            true: start_true_0_true;
            false: start_true_0_false;
        }
    }
    state start_true_0_true {
        tmp_16 = b + 32w1;
        tmp_15 = tmp_16;
        transition start_true_0_join;
    }
    state start_true_0_false {
        tmp_17 = b + 32w2;
        tmp_15 = tmp_17;
        transition start_true_0_join;
    }
    state start_true_0_join {
        tmp_13 = tmp_15;
        transition start_join_0;
    }
    state start_false_0 {
        tmp_18 = b + 32w3;
        tmp_13 = tmp_18;
        transition start_join_0;
    }
    state start_join_0 {
        b = tmp_13;
        transition accept;
    }
}

parser proto();
package top(proto _p);
top(p()) main;
