control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    @hidden action concat15() {
        x = 32w0xf0f1e1e;
    }
    @hidden table tbl_concat15 {
        actions = {
            concat15();
        }
        const default_action = concat15();
    }
    apply {
        tbl_concat15.apply();
    }
}

top(c()) main;
