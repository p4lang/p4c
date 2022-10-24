control c(out bit<32> x);
package top(c _c);
control my(out bit<32> x) {
    @hidden action overwrite24() {
        x = 32w2;
    }
    @hidden table tbl_overwrite24 {
        actions = {
            overwrite24();
        }
        const default_action = overwrite24();
    }
    apply {
        tbl_overwrite24.apply();
    }
}

top(my()) main;
