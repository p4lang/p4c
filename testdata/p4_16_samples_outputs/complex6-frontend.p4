extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_0") bool tmp_0;
    @name("c.tmp_1") bit<32> tmp_1;
    @name("c.tmp_2") bool tmp_2;
    apply {
        tmp = f(32w2);
        tmp_0 = tmp > 32w0;
        if (tmp_0) {
            tmp_1 = f(32w2);
            tmp_2 = tmp_1 < 32w2;
            if (tmp_2) {
                r = 32w1;
            } else {
                r = 32w3;
            }
        } else {
            r = 32w2;
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
