header H {
    bit<32> isValid;
    bit<32> setValid;
}

control c(inout H h) {
    @hidden action specissue1060l9() {
        h.setValid = h.isValid + 32w32;
    }
    @hidden table tbl_specissue1060l9 {
        actions = {
            specissue1060l9();
        }
        const default_action = specissue1060l9();
    }
    apply {
        if (h.isValid()) {
            tbl_specissue1060l9.apply();
        }
    }
}

control C(inout H h);
package top(C c);
top(c()) main;
