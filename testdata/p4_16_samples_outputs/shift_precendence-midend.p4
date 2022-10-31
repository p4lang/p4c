control i(out bit<4> a, out bit<16> x) {
    @hidden action shift_precendence5() {
        a = 4w0;
        x = 16w0xfff;
    }
    @hidden table tbl_shift_precendence5 {
        actions = {
            shift_precendence5();
        }
        const default_action = shift_precendence5();
    }
    apply {
        tbl_shift_precendence5.apply();
    }
}

control c(out bit<4> a, out bit<16> x);
package top(c _c);
top(i()) main;
