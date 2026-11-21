control i(out bit<4> a, out bit<16> x) {
    @hidden action shift_precendence11() {
        a = 4w0;
        x = 16w0xfff;
    }
    @hidden table tbl_shift_precendence11 {
        actions = {
            shift_precendence11();
        }
        const default_action = shift_precendence11();
    }
    apply {
        tbl_shift_precendence11.apply();
    }
}

control c(out bit<4> a, out bit<16> x);
package top(c _c);
top(i()) main;
