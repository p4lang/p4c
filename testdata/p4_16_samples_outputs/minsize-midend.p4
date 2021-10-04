control c(out bit<32> v) {
    @hidden action minsize24() {
        v = 32w64;
    }
    @hidden table tbl_minsize24 {
        actions = {
            minsize24();
        }
        const default_action = minsize24();
    }
    apply {
        tbl_minsize24.apply();
    }
}

control cproto(out bit<32> v);
package top(cproto _c);
top(c()) main;

