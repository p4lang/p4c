control proto(out bit<32> o);
package top(proto _c, bool parameter);
control c(out bit<32> o) {
    @hidden action package21() {
        o = 32w0;
    }
    @hidden table tbl_package21 {
        actions = {
            package21();
        }
        const default_action = package21();
    }
    apply {
        tbl_package21.apply();
    }
}

top(c(), true) main;
