struct S {
    bit<8> x;
}

control c(out bit<16> result) {
    @hidden action constStruct12() {
        result = 16w0;
    }
    @hidden table tbl_constStruct12 {
        actions = {
            constStruct12();
        }
        const default_action = constStruct12();
    }
    apply {
        tbl_constStruct12.apply();
    }
}

control ctrl(out bit<16> result);
package top(ctrl _c);
top(c()) main;
