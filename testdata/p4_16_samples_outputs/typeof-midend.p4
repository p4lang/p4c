control C(out bit<32> x);
package top(C _c);
control impl(out bit<32> x) {
    @hidden action typeof10() {
        x = 32w1;
    }
    @hidden table tbl_typeof10 {
        actions = {
            typeof10();
        }
        const default_action = typeof10();
    }
    apply {
        tbl_typeof10.apply();
    }
}

top(impl()) main;

