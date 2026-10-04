struct S {
    bit<8> x;
}

control c(out bit<16> result) {
    @hidden action constStruct15() {
        result = 16w0;
    }
    @hidden table tbl_constStruct15 {
        actions = {
            constStruct15();
        }
        const default_action = constStruct15();
    }
    apply {
        tbl_constStruct15.apply();
    }
}

control ctrl(out bit<16> result);
package top(ctrl _c);
top(c()) main;
