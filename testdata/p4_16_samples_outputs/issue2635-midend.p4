struct S<T, N> {
    T       field;
    bit<32> otherField;
}

struct S_0 {
    bit<32> field;
    bit<32> otherField;
}

control _c<T>(out T t);
package top<T>(_c<T> c);
control c(out S_0 t) {
    @hidden action issue2635l13() {
        t.field = 32w0;
        t.otherField = 32w0;
    }
    @hidden table tbl_issue2635l13 {
        actions = {
            issue2635l13();
        }
        const default_action = issue2635l13();
    }
    apply {
        tbl_issue2635l13.apply();
    }
}

top<S_0>(c()) main;
