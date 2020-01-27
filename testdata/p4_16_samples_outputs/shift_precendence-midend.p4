control i(out bit<4> a) {
    @hidden action shift_precendence5() {
        a = 4w0;
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

control c(out bit<4> a);
package top(c _c);
top(i()) main;

