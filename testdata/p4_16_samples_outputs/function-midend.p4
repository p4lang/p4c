control c(out bit<16> b) {
    @hidden action function9() {
        b = 16w12;
    }
    @hidden table tbl_function9 {
        actions = {
            function9();
        }
        const default_action = function9();
    }
    apply {
        tbl_function9.apply();
    }
}

control ctr(out bit<16> b);
package top(ctr _c);
top(c()) main;
