control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    @hidden action constant_folding59() {
        x = 32w17;
    }
    @hidden table tbl_constant_folding59 {
        actions = {
            constant_folding59();
        }
        const default_action = constant_folding59();
    }
    apply {
        tbl_constant_folding59.apply();
    }
}

top(c()) main;
