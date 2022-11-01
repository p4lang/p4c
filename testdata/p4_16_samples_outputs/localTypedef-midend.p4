control C(out bit<16> result) {
    @hidden action localTypedef8() {
        result = 16w11;
    }
    @hidden table tbl_localTypedef8 {
        actions = {
            localTypedef8();
        }
        const default_action = localTypedef8();
    }
    apply {
        tbl_localTypedef8.apply();
    }
}

control CT(out bit<16> r);
package top(CT _c);
top(C()) main;

