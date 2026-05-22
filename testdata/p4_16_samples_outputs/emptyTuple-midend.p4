struct tuple_0 {
}

control c(out bool b) {
    @hidden action emptyTuple13() {
        b = true;
    }
    @hidden table tbl_emptyTuple13 {
        actions = {
            emptyTuple13();
        }
        const default_action = emptyTuple13();
    }
    apply {
        tbl_emptyTuple13.apply();
    }
}

control e(out bool b);
package top(e _e);
top(c()) main;
