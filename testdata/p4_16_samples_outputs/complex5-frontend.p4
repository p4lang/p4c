extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp;
    bool tmp_0;
    apply {
        tmp = f(32w2);
        tmp_0 = tmp > 32w0;
        if (tmp_0) 
            r = 32w1;
        else 
            r = 32w2;
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

