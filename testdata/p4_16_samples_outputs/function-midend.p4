control c(out bit<16> b) {
    @hidden action function15() {
        b = 16w12;
    }
    @hidden table tbl_function15 {
        actions = {
            function15();
        }
        const default_action = function15();
    }
    apply {
        tbl_function15.apply();
    }
}

control ctr(out bit<16> b);
package top(ctr _c);
top(c()) main;
