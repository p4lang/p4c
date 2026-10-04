control c(out bool b) {
    @hidden action newtype1l17() {
        b = false;
    }
    @hidden table tbl_newtype1l17 {
        actions = {
            newtype1l17();
        }
        const default_action = newtype1l17();
    }
    apply {
        tbl_newtype1l17.apply();
    }
}

control ctrl(out bool b);
package top(ctrl _c);
top(c()) main;
