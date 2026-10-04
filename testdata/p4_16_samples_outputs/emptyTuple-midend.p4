struct tuple_0 {
}

control c(out bool b) {
    @hidden action emptyTuple15() {
        b = true;
    }
    @hidden table tbl_emptyTuple15 {
        actions = {
            emptyTuple15();
        }
        const default_action = emptyTuple15();
    }
    apply {
        tbl_emptyTuple15.apply();
    }
}

control e(out bool b);
package top(e _e);
top(c()) main;
