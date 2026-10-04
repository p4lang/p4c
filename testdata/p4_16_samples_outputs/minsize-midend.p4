header H {
}

control c(out bit<32> v) {
    @hidden action minsize16() {
        v = 32w128;
    }
    @hidden table tbl_minsize16 {
        actions = {
            minsize16();
        }
        const default_action = minsize16();
    }
    apply {
        tbl_minsize16.apply();
    }
}

control cproto(out bit<32> v);
package top(cproto _c);
top(c()) main;
