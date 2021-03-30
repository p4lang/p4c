control c(inout bit<32> x) {
    @name("c.tmp_0") bit<32> tmp;
    @name("c.tmp_1") bit<32> tmp_0;
    apply {
        {
            @name("c.a_0") bit<32> a_0 = x;
            @name("c.b_0") bit<32> b_0 = x + 32w1;
            @name("c.hasReturned") bool hasReturned = false;
            @name("c.retval") bit<32> retval;
            @name("c.tmp") bit<32> tmp_1;
            if (a_0 > b_0) {
                tmp_1 = b_0;
            } else {
                tmp_1 = a_0;
            }
            hasReturned = true;
            retval = tmp_1;
            tmp = retval;
        }
        {
            @name("c.a_1") bit<32> a_1 = x;
            @name("c.b_1") bit<32> b_1 = x + 32w4294967295;
            @name("c.hasReturned") bool hasReturned_1 = false;
            @name("c.retval") bit<32> retval_1;
            @name("c.tmp") bit<32> tmp_5;
            if (a_1 > b_1) {
                tmp_5 = b_1;
            } else {
                tmp_5 = a_1;
            }
            hasReturned_1 = true;
            retval_1 = tmp_5;
            tmp_0 = retval_1;
        }
        {
            @name("c.a_2") bit<32> a_2 = tmp;
            @name("c.b_2") bit<32> b_2 = tmp_0;
            @name("c.hasReturned") bool hasReturned_2 = false;
            @name("c.retval") bit<32> retval_2;
            @name("c.tmp") bit<32> tmp_6;
            if (a_2 > b_2) {
                tmp_6 = b_2;
            } else {
                tmp_6 = a_2;
            }
            hasReturned_2 = true;
            retval_2 = tmp_6;
            x = retval_2;
        }
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;

