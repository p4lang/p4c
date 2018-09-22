control c(out bit<16> b) {
    bit<16> tmp_0;
    apply {
        {
            bit<16> left = 16w10;
            bit<16> right = 16w12;
            bool hasReturned_0 = false;
            bit<16> retval_0;
            if (left > right) {
                hasReturned_0 = true;
                retval_0 = left;
            }
            if (!hasReturned_0) {
                hasReturned_0 = true;
                retval_0 = right;
            }
            tmp_0 = retval_0;
        }
        b = tmp_0;
    }
}

control ctr(out bit<16> b);
package top(ctr _c);
top(c()) main;

