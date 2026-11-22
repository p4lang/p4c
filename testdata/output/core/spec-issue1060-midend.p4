header H {
    bit<32> isValid;
    bit<32> setValid;
}

control c(inout H h) {
    @hidden action specissue1060l15() {
        h.setValid = h.isValid + 32w32;
    }
    @hidden table tbl_specissue1060l15 {
        actions = {
            specissue1060l15();
        }
        const default_action = specissue1060l15();
    }
    apply {
        if (h.isValid()) {
            tbl_specissue1060l15.apply();
        }
    }
}

control C(inout H h);
package top(C c);
top(c()) main;
