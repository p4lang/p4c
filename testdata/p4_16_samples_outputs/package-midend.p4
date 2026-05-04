control proto(out bit<32> o);
package top(proto _c, bool parameter);
control c(out bit<32> o) {
    @hidden action package12() {
        o = 32w0;
    }
    @hidden table tbl_package12 {
        actions = {
            package12();
        }
        const default_action = package12();
    }
    apply {
        tbl_package12.apply();
    }
}

top(c(), true) main;
