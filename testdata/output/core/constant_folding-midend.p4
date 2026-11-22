control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    @hidden action constant_folding50() {
        x = 32w17;
    }
    @hidden table tbl_constant_folding50 {
        actions = {
            constant_folding50();
        }
        const default_action = constant_folding50();
    }
    apply {
        tbl_constant_folding50.apply();
    }
}

top(c()) main;
