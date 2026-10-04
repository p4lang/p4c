extern void f(in bit<16> x, in bool y);
control c() {
    @hidden action namedarg12() {
        f(y = true, x = 16w0);
    }
    @hidden table tbl_namedarg12 {
        actions = {
            namedarg12();
        }
        const default_action = namedarg12();
    }
    apply {
        tbl_namedarg12.apply();
    }
}

control empty();
package top(empty _e);
top(c()) main;
