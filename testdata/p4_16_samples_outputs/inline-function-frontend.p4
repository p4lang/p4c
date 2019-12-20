control c(inout bit<32> x) {
    apply {
        {
            bit<32> a_0 = x;
            bit<32> b_0 = x;
            bool hasReturned = false;
            bit<32> retval;
            bit<32> tmp;
            bit<32> tmp_0;
            {
                bit<32> a_1 = a_0;
                bit<32> b_1 = b_0;
                bool hasReturned_0 = false;
                bit<32> retval_0;
                bit<32> tmp_1;
                if (a_1 > b_1) {
                    tmp_1 = b_1;
                } else {
                    tmp_1 = a_1;
                }
                hasReturned_0 = true;
                retval_0 = tmp_1;
                tmp = retval_0;
            }
            tmp_0 = a_0 + tmp;
            hasReturned = true;
            retval = tmp_0;
            x = retval;
        }
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;

