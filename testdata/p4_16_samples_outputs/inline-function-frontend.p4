control c(inout bit<32> x) {
    apply {
        {
            @name("c.a_1") bit<32> a_0 = x;
            @name("c.b_1") bit<32> b_0 = x;
            @name("c.hasReturned_0") bool hasReturned = false;
            @name("c.retval_0") bit<32> retval;
            @name("c.tmp_0") bit<32> tmp;
            @name("c.tmp_1") bit<32> tmp_0;
            @name("c.tmp_2") bit<32> tmp_1;
            tmp = a_0;
            {
                @name("c.a_0") bit<32> a_1 = a_0;
                @name("c.b_0") bit<32> b_1 = b_0;
                @name("c.hasReturned") bool hasReturned_0 = false;
                @name("c.retval") bit<32> retval_0;
                @name("c.tmp") bit<32> tmp_2;
                if (a_1 > b_1) {
                    tmp_2 = b_1;
                } else {
                    tmp_2 = a_1;
                }
                hasReturned_0 = true;
                retval_0 = tmp_2;
                tmp_0 = retval_0;
            }
            tmp_1 = tmp + tmp_0;
            hasReturned = true;
            retval = tmp_1;
            x = retval;
        }
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;

