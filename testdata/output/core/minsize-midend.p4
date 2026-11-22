header H {
}

control c(out bit<32> v) {
    @hidden action minsize18() {
        v = 32w128;
    }
    @hidden table tbl_minsize18 {
        actions = {
            minsize18();
        }
        const default_action = minsize18();
    }
    apply {
        tbl_minsize18.apply();
    }
}

control cproto(out bit<32> v);
package top(cproto _c);
top(c()) main;
