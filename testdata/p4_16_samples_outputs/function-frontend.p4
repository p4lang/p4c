control c(out bit<16> b) {
    bit<16> tmp;
    apply {
        {
            bit<16> left_0 = 16w10;
            bit<16> right_0 = 16w12;
            bool hasReturned = false;
            bit<16> retval;
            if (left_0 > right_0) {
                hasReturned = true;
                retval = left_0;
            }
            if (!hasReturned) {
                hasReturned = true;
                retval = right_0;
            }
            tmp = retval;
        }
        b = tmp;
    }
}

control ctr(out bit<16> b);
package top(ctr _c);
top(c()) main;

