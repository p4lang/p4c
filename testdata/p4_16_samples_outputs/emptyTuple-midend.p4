struct tuple_0 {
}

control c(out bool b) {
    @hidden action emptyTuple7() {
        b = true;
    }
    @hidden table tbl_emptyTuple7 {
        actions = {
            emptyTuple7();
        }
        const default_action = emptyTuple7();
    }
    apply {
        tbl_emptyTuple7.apply();
    }
}

control e(out bool b);
package top(e _e);
top(c()) main;
