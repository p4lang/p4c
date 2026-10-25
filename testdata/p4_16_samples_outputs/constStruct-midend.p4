struct S {
    bit<8> x;
}

control c(out bit<16> result) {
    @hidden action constStruct18() {
        result = 16w0;
    }
    @hidden table tbl_constStruct18 {
        actions = {
            constStruct18();
        }
        const default_action = constStruct18();
    }
    apply {
        tbl_constStruct18.apply();
    }
}

control ctrl(out bit<16> result);
package top(ctrl _c);
top(c()) main;
