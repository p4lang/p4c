extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp_2;
    bit<32> tmp_3;
    bit<32> tmp_4;
    apply {
        tmp_2 = f(32w5);
        tmp_3 = tmp_2;
        tmp_4 = f(tmp_3);
        r = tmp_4;
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

