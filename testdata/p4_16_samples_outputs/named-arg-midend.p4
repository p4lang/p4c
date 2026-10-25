extern void f(in bit<16> x, in bool y);
control c() {
    @hidden action namedarg14() {
        f(y = true, x = 16w0);
    }
    @hidden table tbl_namedarg14 {
        actions = {
            namedarg14();
        }
        const default_action = namedarg14();
    }
    apply {
        tbl_namedarg14.apply();
    }
}

control empty();
package top(empty _e);
top(c()) main;
