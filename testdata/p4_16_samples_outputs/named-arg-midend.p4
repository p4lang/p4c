extern void f(in bit<16> x, in bool y);
control c() {
    @hidden action namedarg8() {
        f(y = true, x = 16w0);
    }
    @hidden table tbl_namedarg8 {
        actions = {
            namedarg8();
        }
        const default_action = namedarg8();
    }
    apply {
        tbl_namedarg8.apply();
    }
}

control empty();
package top(empty _e);
top(c()) main;
