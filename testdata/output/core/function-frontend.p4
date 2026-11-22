control c(out bit<16> b) {
    @name("c.left_0") bit<16> left;
    @name("c.right_0") bit<16> right;
    @name("c.retval") bit<16> retval;
    @name("c.inlinedRetval") bit<16> inlinedRetval_0;
    apply {
        left = 16w10;
        right = 16w12;
        if (left > right) {
            retval = left;
        } else {
            retval = right;
        }
        inlinedRetval_0 = retval;
        b = inlinedRetval_0;
    }
}

control ctr(out bit<16> b);
package top(ctr _c);
top(c()) main;
