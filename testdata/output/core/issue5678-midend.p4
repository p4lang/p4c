struct S {
    bit<8> x;
}

control c(out bit<8> result) {
    @hidden action issue5678l13() {
        result = 8w1;
    }
    @hidden table tbl_issue5678l13 {
        actions = {
            issue5678l13();
        }
        const default_action = issue5678l13();
    }
    apply {
        tbl_issue5678l13.apply();
    }
}

control ctrl(out bit<8> result);
package top(ctrl _c);
top(c()) main;
