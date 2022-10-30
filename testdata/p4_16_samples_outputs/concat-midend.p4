control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    @hidden action concat24() {
        x = 32w0xf0f1e1e;
    }
    @hidden table tbl_concat24 {
        actions = {
            concat24();
        }
        const default_action = concat24();
    }
    apply {
        tbl_concat24.apply();
    }
}

top(c()) main;
