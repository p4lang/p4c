struct S {
    bit<32> l;
    bit<32> r;
}

control c(out bool z);
package top(c _c);
struct tuple_0 {
    bit<32> f0;
    bit<32> f1;
}

control test(out bool zout) {
    @hidden action listcompare28() {
        zout = true;
        zout = true;
    }
    @hidden table tbl_listcompare28 {
        actions = {
            listcompare28();
        }
        const default_action = listcompare28();
    }
    apply {
        tbl_listcompare28.apply();
    }
}

top(test()) main;
