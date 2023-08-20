control c(inout bit<32> x) {
    @name("c.tmp_0") bit<32> tmp;
    @name("c.tmp_1") bit<32> tmp_0;
    @name("c.a_0") bit<32> a;
    @name("c.b_0") bit<32> b;
    @name("c.retval") bit<32> retval;
    @name("c.tmp") bit<32> tmp_1;
    @name("c.a_1") bit<32> a_3;
    @name("c.b_1") bit<32> b_3;
    @name("c.retval") bit<32> retval_1;
    @name("c.tmp") bit<32> tmp_5;
    @name("c.a_2") bit<32> a_4;
    @name("c.b_2") bit<32> b_4;
    @name("c.retval") bit<32> retval_2;
    @name("c.tmp") bit<32> tmp_6;
    apply {
        a = x;
        b = x + 32w1;
        if (a > b) {
            tmp_1 = b;
        } else {
            tmp_1 = a;
        }
        retval = tmp_1;
        tmp = retval;
        a_3 = x;
        b_3 = x + 32w4294967295;
        if (a_3 > b_3) {
            tmp_5 = b_3;
        } else {
            tmp_5 = a_3;
        }
        retval_1 = tmp_5;
        tmp_0 = retval_1;
        a_4 = tmp;
        b_4 = tmp_0;
        if (a_4 > b_4) {
            tmp_6 = b_4;
        } else {
            tmp_6 = a_4;
        }
        retval_2 = tmp_6;
        x = retval_2;
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;
