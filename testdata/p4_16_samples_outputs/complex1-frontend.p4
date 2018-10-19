extern bit<32> f(in bit<32> x, in bit<32> y);
control c(inout bit<32> r) {
    bit<32> tmp;
    bit<32> tmp_0;
    bit<32> tmp_1;
    bit<32> tmp_2;
    bit<32> tmp_3;
    bit<32> tmp_4;
    bit<32> tmp_5;
    apply {
        tmp = f(32w5, 32w2);
        tmp_0 = tmp;
        tmp_1 = f(32w2, 32w3);
        tmp_2 = tmp_1;
        tmp_3 = f(32w6, tmp_2);
        tmp_4 = tmp_3;
        tmp_5 = f(tmp_0, tmp_4);
        r = tmp_5;
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

