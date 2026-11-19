struct S {
    bit<8>  a;
    bit<16> b;
}

struct N {
    S    s;
    bool f;
}

control c(out bool z);
package top(c _c);
control test(out bool zout) {
    @hidden action structcompare41() {
        zout = true;
    }
    @hidden table tbl_structcompare41 {
        actions = {
            structcompare41();
        }
        const default_action = structcompare41();
    }
    apply {
        tbl_structcompare41.apply();
    }
}

top(test()) main;
