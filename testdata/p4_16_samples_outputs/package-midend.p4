control proto(out bit<32> o);
package top(proto _c, bool parameter);
control c(out bit<32> o) {
    @hidden action package13() {
        o = 32w0;
    }
    @hidden table tbl_package13 {
        actions = {
            package13();
        }
        const default_action = package13();
    }
    apply {
        tbl_package13.apply();
    }
}

top(c(), true) main;
