control c(out bool b) {
    @hidden action newtype1l18() {
        b = false;
    }
    @hidden table tbl_newtype1l18 {
        actions = {
            newtype1l18();
        }
        const default_action = newtype1l18();
    }
    apply {
        tbl_newtype1l18.apply();
    }
}

control ctrl(out bool b);
package top(ctrl _c);
top(c()) main;
