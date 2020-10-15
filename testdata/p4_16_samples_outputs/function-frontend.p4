control c(out bit<16> b) {
    apply {
        {
            @name("c.left_0") bit<16> left_0 = 16w10;
            @name("c.right_0") bit<16> right_0 = 16w12;
            @name("c.hasReturned") bool hasReturned = false;
            @name("c.retval") bit<16> retval;
            if (left_0 > right_0) {
                hasReturned = true;
                retval = left_0;
            }
            if (!hasReturned) {
                hasReturned = true;
                retval = right_0;
            }
            b = retval;
        }
    }
}

control ctr(out bit<16> b);
package top(ctr _c);
top(c()) main;

