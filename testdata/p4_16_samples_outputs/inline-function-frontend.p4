control c(inout bit<32> x) {
    bit<32> tmp_3;
    apply {
        {
            bit<32> a = x;
            bit<32> b = x;
            bool hasReturned_1 = false;
            bit<32> retval_1;
            bit<32> tmp_4;
            bit<32> tmp_5;
            {
                bit<32> a_2 = a;
                bit<32> b_2 = b;
                bool hasReturned_2 = false;
                bit<32> retval_2;
                bit<32> tmp_6;
                if (a_2 > b_2) 
                    tmp_6 = b_2;
                else 
                    tmp_6 = a_2;
                hasReturned_2 = true;
                retval_2 = tmp_6;
                tmp_4 = retval_2;
            }
            tmp_5 = a + tmp_4;
            hasReturned_1 = true;
            retval_1 = tmp_5;
            tmp_3 = retval_1;
        }
        x = tmp_3;
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;

