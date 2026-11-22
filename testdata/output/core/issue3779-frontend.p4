header h {
}

control c(out bool b) {
    @name("c.retval") bool retval;
    @name("c.inlinedRetval") bool inlinedRetval_0;
    apply {
        retval = true;
        inlinedRetval_0 = retval;
        b = inlinedRetval_0;
    }
}

control C(out bool b);
package top(C _c);
top(c()) main;
