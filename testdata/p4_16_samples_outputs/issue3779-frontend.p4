header h {
}

control c(out bool b) {
    @name("c.retval") bool retval;
    apply {
        retval = true;
        b = retval;
    }
}

control C(out bool b);
package top(C _c);
top(c()) main;
