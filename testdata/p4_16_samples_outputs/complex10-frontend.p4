extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp;
    bool tmp_0;
    bool tmp_1;
    bit<32> tmp_2;
    bool tmp_3;
    bool tmp_4;
    bit<32> tmp_5;
    bool tmp_6;
    apply {
        tmp = f(32w2);
        tmp_0 = tmp > 32w0;
        if (!tmp_0) {
            tmp_1 = false;
        } else {
            tmp_2 = f(32w3);
            tmp_3 = tmp_2 < 32w0;
            tmp_1 = tmp_3;
        }
        if (tmp_1) {
            tmp_4 = true;
        } else {
            tmp_5 = f(32w5);
            tmp_6 = tmp_5 == 32w2;
            tmp_4 = tmp_6;
        }
        if (tmp_4) {
            r = 32w1;
        } else {
            r = 32w2;
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

