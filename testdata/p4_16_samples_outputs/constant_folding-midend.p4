control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    @hidden action constant_folding38() {
        x = 32w17;
    }
    @hidden table tbl_constant_folding38 {
        actions = {
            constant_folding38();
        }
        const default_action = constant_folding38();
    }
    apply {
        tbl_constant_folding38.apply();
    }
}

top(c()) main;
