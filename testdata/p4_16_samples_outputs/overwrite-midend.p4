control c(out bit<32> x);
package top(c _c);
control my(out bit<32> x) {
    @hidden action overwrite14() {
        x = 32w2;
    }
    @hidden table tbl_overwrite14 {
        actions = {
            overwrite14();
        }
        const default_action = overwrite14();
    }
    apply {
        tbl_overwrite14.apply();
    }
}

top(my()) main;
