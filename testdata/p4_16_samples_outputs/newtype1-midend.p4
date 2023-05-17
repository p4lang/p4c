control c(out bool b) {
    @hidden action newtype1l12() {
        b = false;
    }
    @hidden table tbl_newtype1l12 {
        actions = {
            newtype1l12();
        }
        const default_action = newtype1l12();
    }
    apply {
        tbl_newtype1l12.apply();
    }
}

control ctrl(out bool b);
package top(ctrl _c);
top(c()) main;
