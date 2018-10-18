control c(inout bit<32> x) {
    bit<32> tmp_5;
    bit<32> tmp_6;
    bit<32> tmp_7;
    bit<32> tmp_8;
    bit<32> tmp_9;
    apply {
        {
            bit<32> a = x;
            bit<32> b = x + 32w1;
            bool hasReturned_0 = false;
            bit<32> retval_0;
            bit<32> tmp_10;
            if (a > b) 
                tmp_10 = b;
            else 
                tmp_10 = a;
            hasReturned_0 = true;
            retval_0 = tmp_10;
            tmp_5 = retval_0;
        }
        tmp_6 = tmp_5;
        {
            bit<32> a_3 = x;
            bit<32> b_3 = x + 32w4294967295;
            bool hasReturned_0 = false;
            bit<32> retval_0;
            bit<32> tmp_10;
            if (a_3 > b_3) 
                tmp_10 = b_3;
            else 
                tmp_10 = a_3;
            hasReturned_0 = true;
            retval_0 = tmp_10;
            tmp_7 = retval_0;
        }
        tmp_8 = tmp_7;
        {
            bit<32> a_4 = tmp_6;
            bit<32> b_4 = tmp_8;
            bool hasReturned_0 = false;
            bit<32> retval_0;
            bit<32> tmp_10;
            if (a_4 > b_4) 
                tmp_10 = b_4;
            else 
                tmp_10 = a_4;
            hasReturned_0 = true;
            retval_0 = tmp_10;
            tmp_9 = retval_0;
        }
        x = tmp_9;
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;

