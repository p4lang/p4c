control c(out bit<32> x);
package top(c _c);
control my(out bit<32> x) {
    @hidden action overwrite15() {
        x = 32w2;
    }
    @hidden table tbl_overwrite15 {
        actions = {
            overwrite15();
        }
        const default_action = overwrite15();
    }
    apply {
        tbl_overwrite15.apply();
    }
}

top(my()) main;
