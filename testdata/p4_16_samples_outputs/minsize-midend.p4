header H {
}

control c(out bit<32> v) {
    @hidden action minsize27() {
        v = 32w128;
    }
    @hidden table tbl_minsize27 {
        actions = {
            minsize27();
        }
        const default_action = minsize27();
    }
    apply {
        tbl_minsize27.apply();
    }
}

control cproto(out bit<32> v);
package top(cproto _c);
top(c()) main;
