extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp_1;
    bool tmp_2;
    apply {
        tmp_1 = f(32w2);
        tmp_2 = tmp_1 > 32w0;
        if (tmp_2) 
            r = 32w1;
        else 
            r = 32w2;
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

